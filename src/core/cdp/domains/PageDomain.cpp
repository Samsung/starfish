/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#if defined(STARFISH_ENABLE_CDP)

#include "StarfishConfig.h"
#include "Starfish.h"
#include "PageDomain.h"
#include "NetworkDomain.h"
#include "FetchDomain.h"
#include "RuntimeDomain.h"
#include "../CDPDispatcher.h"
#include "../CDPCommand.h"
#include "../CDPSession.h"
#include "../NodeRegistry.h"
#include "../RemoteObject.h"
#include "../Base64.h"
#include "core/page/WebView.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/modules/message_loop/Timer.h"
#include "browser/history/HistoryManager.h"
#include "platform/loader/ResourceURL.h"
#include "core/dom/Document.h"
#include "core/csp/ContentSecurityPolicy.h"
#include "core/modules/canvas/image/ImageEncoder.h"
#include "core/modules/renderer/Renderer.h"
#include "core/layout/LayoutUtil.h"

#include "rapidjson/document.h"

#include <cmath>

#if defined(PORT_CANVAS_BACKEND_CAIRO)
#include <cairo.h>
#include <cairo-pdf.h>
#endif

namespace Starfish {

static std::string currentUrl(WebView* wv)
{
    BrowsingContext* bc = wv->mainBrowsingContext();
    if (bc && bc->document()) {
        String* u = bc->document()->urlString();
        if (u) {
            return u->toUTF8NonGCString();
        }
    }
    return "about:blank";
}

static std::string currentOrigin(WebView* wv)
{
    BrowsingContext* bc = wv->mainBrowsingContext();
    if (bc && bc->document()) {
        String* o = bc->document()->origin();
        if (o) {
            return o->toUTF8NonGCString();
        }
    }
    return "";
}

// Resolve the main document's <link rel="manifest" href="..."> to an absolute
// URL (href resolved against the document base URI). Empty if there is no such
// link or it has no href. Used by Page.getAppManifest.
static std::string manifestUrl(WebView* wv)
{
    BrowsingContext* bc = wv ? wv->mainBrowsingContext() : nullptr;
    if (!bc || !bc->document()) {
        return std::string();
    }
    Document* doc = bc->document();
    // rel is a space-separated token list; ~= matches "manifest" as one token.
    Element* link =
        doc->querySelector(String::fromUTF8("link[rel~=\"manifest\"]"));
    if (!link) {
        return std::string();
    }
    Optional<String*> href = link->getAttribute(String::fromUTF8("href"));
    if (!href.hasValue() || href.getValue()->isEmpty()) {
        return std::string();
    }
    String* base = doc->baseURL() ? doc->baseURL()->baseURI() : nullptr;
    ResourceURL resolved(href.getValue(), base);
    String* abs = resolved.urlString();
    if (abs) {
        return abs->toUTF8NonGCString();
    }
    return href.getValue()->toUTF8NonGCString();
}

// Serialize the main frame's current document via XMLSerializer
// (documentElement.outerHTML), prefixed with a doctype. Empty if no document.
static std::string mainDocumentHTML(WebView* wv)
{
    BrowsingContext* bc = wv ? wv->mainBrowsingContext() : nullptr;
    if (bc && bc->document() && bc->document()->documentElement()) {
        String* html = bc->document()->documentElement()->outerHTML();
        if (html) {
            return "<!DOCTYPE html>\n" + html->toUTF8NonGCString();
        }
    }
    return std::string();
}

// Full content (document) height in CSS px for the main frame, used to drive
// PDF pagination. Uses the root element's scrollHeight (the laid-out content
// height, as exposed to JS), so a short page stays one page while long content
// paginates. Returns 0 when there is no laid-out document. CSS px maps 1:1 to
// PDF points at scale 1.
static double mainContentHeightCssPx(WebView* wv)
{
    BrowsingContext* bc = wv ? wv->mainBrowsingContext() : nullptr;
    if (bc && bc->document() && bc->document()->documentElement()) {
        return (double)bc->document()->documentElement()->scrollHeight();
    }
    return 0.0;
}

// Parsed Page.printToPDF options, normalized to PDF points (1in = 72pt).
// Defaults match the CDP/Chrome contract: US Letter (8.5x11in), 0.4in margins,
// scale 1, portrait.
struct PrintToPdfOptions {
    double paperWidthPts = 8.5 * 72; // 612
    double paperHeightPts = 11 * 72; // 792
    double marginTopPts = 0.4 * 72;
    double marginBottomPts = 0.4 * 72;
    double marginLeftPts = 0.4 * 72;
    double marginRightPts = 0.4 * 72;
    double scale = 1.0;
    bool landscape = false;
    std::string pageRanges; // e.g. "1-2,4"; empty = all pages

    // Printable area on a page after margins (post-landscape paper dims).
    double printableWidthPts() const
    {
        double w = (landscape ? paperHeightPts : paperWidthPts) -
                   marginLeftPts - marginRightPts;
        return w > 1 ? w : 1;
    }
    double printableHeightPts() const
    {
        double h = (landscape ? paperWidthPts : paperHeightPts) - marginTopPts -
                   marginBottomPts;
        return h > 1 ? h : 1;
    }
    double pageWidthPts() const
    {
        return landscape ? paperHeightPts : paperWidthPts;
    }
    double pageHeightPts() const
    {
        return landscape ? paperWidthPts : paperHeightPts;
    }
};

static PrintToPdfOptions parsePrintToPdfOptions(const rapidjson::Value* p)
{
    PrintToPdfOptions o;
    if (!p) {
        return o;
    }
    auto inchToPts = [&](const char* key, double& field) {
        if (p->HasMember(key) && (*p)[key].IsNumber()) {
            field = (*p)[key].GetDouble() * 72.0;
        }
    };
    inchToPts("paperWidth", o.paperWidthPts);
    inchToPts("paperHeight", o.paperHeightPts);
    inchToPts("marginTop", o.marginTopPts);
    inchToPts("marginBottom", o.marginBottomPts);
    inchToPts("marginLeft", o.marginLeftPts);
    inchToPts("marginRight", o.marginRightPts);
    if (p->HasMember("scale") && (*p)["scale"].IsNumber()) {
        double s = (*p)["scale"].GetDouble();
        if (s > 0) {
            o.scale = s;
        }
    }
    if (p->HasMember("landscape") && (*p)["landscape"].IsBool()) {
        o.landscape = (*p)["landscape"].GetBool();
    }
    if (p->HasMember("pageRanges") && (*p)["pageRanges"].IsString()) {
        o.pageRanges = (*p)["pageRanges"].GetString();
    }
    // Guard against degenerate/negative paper sizes.
    if (o.paperWidthPts < 1) {
        o.paperWidthPts = 1;
    }
    if (o.paperHeightPts < 1) {
        o.paperHeightPts = 1;
    }
    return o;
}

// Decide whether 1-based page index `page` (out of `total`) should be emitted,
// given a CDP pageRanges string ("1-2,4", empty = all). Tolerant parser: only
// well-formed "N" / "N-M" tokens are honored; garbage tokens are ignored.
static bool pageInRanges(const std::string& ranges, int page, int total)
{
    if (ranges.empty()) {
        return true;
    }
    size_t i = 0;
    while (i < ranges.size()) {
        size_t comma = ranges.find(',', i);
        std::string tok = ranges.substr(
            i, comma == std::string::npos ? std::string::npos : comma - i);
        i = (comma == std::string::npos) ? ranges.size() : comma + 1;
        // trim spaces
        size_t a = tok.find_first_not_of(" \t");
        size_t b = tok.find_last_not_of(" \t");
        if (a == std::string::npos) {
            continue;
        }
        tok = tok.substr(a, b - a + 1);
        size_t dash = tok.find('-');
        try {
            if (dash == std::string::npos) {
                if (std::stoi(tok) == page) {
                    return true;
                }
            } else {
                std::string ls = tok.substr(0, dash);
                std::string rs = tok.substr(dash + 1);
                int lo = ls.empty() ? 1 : std::stoi(ls);
                int hi = rs.empty() ? total : std::stoi(rs);
                if (page >= lo && page <= hi) {
                    return true;
                }
            }
        } catch (...) {
            // ignore malformed token
        }
    }
    return false;
}

#if defined(PORT_CANVAS_BACKEND_CAIRO)
// cairo_write_func_t closure: append the PDF stream bytes to a vector.
static cairo_status_t pdfStreamWrite(void* closure, const unsigned char* data,
                                     unsigned int length)
{
    std::vector<uint8_t>* out = static_cast<std::vector<uint8_t>*>(closure);
    out->insert(out->end(), data, data + length);
    return CAIRO_STATUS_SUCCESS;
}

// Render an RGBA8 (top-to-bottom) viewport bitmap into a PDF that honors the
// printToPDF options: PDF pages are sized to the paper (inch*72, swapped for
// landscape); the bitmap is placed inside the printable area (page minus
// margins), scaled by devicePixelRatio (so device px -> CSS px) times the
// requested `scale`. The number of pages is driven by the full document content
// height (`contentHeightCssPx`, CSS px) vs one printable area; pageRanges
// selects which slices to emit. The captured bitmap covers only the viewport,
// so slices past the captured region render blank -- full-document content
// across pages is bounded by the viewport readback (documented limitation).
// Returns the PDF bytes, or an empty vector on failure.
//
// `dpr` is the device-pixel-ratio used when capturing `rgba` (device px per CSS
// px). The bitmap is `w` x `h` device pixels.
static std::vector<uint8_t> buildRasterPdf(const std::vector<uint8_t>& rgba,
                                           uint32_t w, uint32_t h, double dpr,
                                           double contentHeightCssPx,
                                           const PrintToPdfOptions& opt)
{
    std::vector<uint8_t> pdf;
    if (w == 0 || h == 0) {
        return pdf;
    }
    if (dpr <= 0) {
        dpr = 1;
    }

    // cairo image surfaces store pixels as premultiplied native-endian ARGB.
    // The page is opaque, so use RGB24 and pack the readback RGBA into the
    // surface's BGRX byte order (little-endian: B,G,R,X).
    int stride = cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, (int)w);
    std::vector<uint8_t> img((size_t)stride * h, 0);
    for (uint32_t y = 0; y < h; y++) {
        const uint8_t* src = &rgba[(size_t)y * w * 4];
        uint8_t* dst = &img[(size_t)y * stride];
        for (uint32_t x = 0; x < w; x++) {
            uint8_t r = src[x * 4 + 0];
            uint8_t g = src[x * 4 + 1];
            uint8_t b = src[x * 4 + 2];
            dst[x * 4 + 0] = b;
            dst[x * 4 + 1] = g;
            dst[x * 4 + 2] = r;
            dst[x * 4 + 3] = 0xff;
        }
    }

    cairo_surface_t* imgSurface = cairo_image_surface_create_for_data(
        img.data(), CAIRO_FORMAT_RGB24, (int)w, (int)h, stride);
    if (cairo_surface_status(imgSurface) != CAIRO_STATUS_SUCCESS) {
        cairo_surface_destroy(imgSurface);
        return pdf;
    }

    cairo_surface_t* pdfSurface = cairo_pdf_surface_create_for_stream(
        pdfStreamWrite, &pdf, opt.pageWidthPts(), opt.pageHeightPts());
    if (cairo_surface_status(pdfSurface) != CAIRO_STATUS_SUCCESS) {
        cairo_surface_destroy(pdfSurface);
        cairo_surface_destroy(imgSurface);
        pdf.clear();
        return pdf;
    }

    cairo_t* cr = cairo_create(pdfSurface);

    // Map device pixels -> PDF points. One CSS px == 1 PDF point at scale 1;
    // device px == CSS px * dpr, so the bitmap's point size is (w/dpr, h/dpr).
    // The requested `scale` further multiplies content size.
    double pxToPt = opt.scale / dpr;

    double printW = opt.printableWidthPts();
    double printH = opt.printableHeightPts();

    // Pages are vertical slices of the content. Page count is driven by the
    // full document content height (CSS px -> pt via scale) vs one printable
    // area; width is not sliced (overflow is clipped).
    double docHPts = contentHeightCssPx * opt.scale;
    if (docHPts < 1) {
        docHPts = (double)h * pxToPt; // fall back to the captured bitmap
    }
    int totalPages = (int)std::ceil(docHPts / printH);
    if (totalPages < 1) {
        totalPages = 1;
    }

    bool anyEmitted = false;
    for (int page = 1; page <= totalPages; page++) {
        if (!pageInRanges(opt.pageRanges, page, totalPages)) {
            continue;
        }
        cairo_save(cr);
        // Clip to the printable area on this page.
        cairo_rectangle(cr, opt.marginLeftPts, opt.marginTopPts, printW,
                        printH);
        cairo_clip(cr);
        // Translate so this slice of content lands at the top of the printable
        // area, then scale device px -> pt.
        double yOffsetPts = (double)(page - 1) * printH;
        cairo_translate(cr, opt.marginLeftPts, opt.marginTopPts - yOffsetPts);
        cairo_scale(cr, pxToPt, pxToPt);
        cairo_set_source_surface(cr, imgSurface, 0, 0);
        cairo_paint(cr);
        cairo_restore(cr);
        cairo_show_page(cr);
        anyEmitted = true;
    }
    // If pageRanges excluded everything, still emit one blank page so the
    // result is a valid PDF.
    if (!anyEmitted) {
        cairo_show_page(cr);
    }

    cairo_destroy(cr);

    cairo_surface_finish(pdfSurface);
    cairo_surface_destroy(pdfSurface);
    cairo_surface_destroy(imgSurface);
    return pdf;
}
#endif // PORT_CANVAS_BACKEND_CAIRO

// Minimal hand-written multi-page PDF with blank pages of the given point size.
// Used by the Mock/headless backend (no cairo, no real pixels) so that the
// CDP/puppeteer contract (a structurally valid %PDF) holds and the page
// size/count reflect the requested options for structural verification.
static std::vector<uint8_t> buildFallbackPdf(double widthPts, double heightPts,
                                             int pageCount = 1)
{
    if (pageCount < 1) {
        pageCount = 1;
    }
    char mediaBox[128];
    snprintf(mediaBox, sizeof(mediaBox), "[0 0 %d %d]", (int)widthPts,
             (int)heightPts);

    // Object layout: 1=Catalog, 2=Pages, 3..(2+pageCount)=Page objects.
    std::vector<std::string> objs;
    objs.push_back("<< /Type /Catalog /Pages 2 0 R >>");

    std::string kids;
    for (int i = 0; i < pageCount; i++) {
        if (i) {
            kids += " ";
        }
        kids += std::to_string(3 + i) + " 0 R";
    }
    objs.push_back("<< /Type /Pages /Kids [" + kids + "] /Count " +
                   std::to_string(pageCount) + " >>");
    for (int i = 0; i < pageCount; i++) {
        objs.push_back(std::string("<< /Type /Page /Parent 2 0 R /MediaBox ") +
                       mediaBox + " /Resources << >> >>");
    }

    std::string body = "%PDF-1.4\n";
    std::vector<size_t> offsets(objs.size() + 1, 0);
    for (size_t i = 0; i < objs.size(); i++) {
        offsets[i + 1] = body.size();
        body += std::to_string(i + 1) + " 0 obj\n" + objs[i] + "\nendobj\n";
    }

    size_t xrefOffset = body.size();
    body += "xref\n0 " + std::to_string(objs.size() + 1) + "\n";
    body += "0000000000 65535 f \n";
    for (size_t i = 1; i <= objs.size(); i++) {
        char ent[32];
        snprintf(ent, sizeof(ent), "%010zu 00000 n \n", offsets[i]);
        body += ent;
    }
    body += "trailer\n<< /Size " + std::to_string(objs.size() + 1) +
            " /Root 1 0 R >>\nstartxref\n" + std::to_string(xrefOffset) +
            "\n%%EOF\n";

    return std::vector<uint8_t>(body.begin(), body.end());
}

// Read back the current document's framebuffer (GL backend) and PNG-encode it,
// reporting the encoded bytes plus the output dimensions in `outW`/`outH`. The
// Mock/headless backend has no rasterizer, so it yields a correctly sized,
// transparent buffer -- a structurally valid (empty) PNG, matching
// captureScreenshot's headless behaviour. jpeg requests fall back to PNG where
// libjpeg is not linked (the same caveat captureScreenshot documents).
static std::vector<uint8_t> captureViewportPng(WebView* wv, uint32_t& outW,
                                               uint32_t& outH)
{
    float dpr = wv->screenInfo().devicePixelRatio;
    if (dpr <= 0) {
        dpr = 1;
    }
    int viewW = wv->screenInfo().rect.size().width();
    int viewH = wv->screenInfo().rect.size().height();

    wv->layoutIfNeeded(false);

    uint32_t w = (uint32_t)(viewW * dpr);
    uint32_t h = (uint32_t)(viewH * dpr);
    if (w == 0) {
        w = 1;
    }
    if (h == 0) {
        h = 1;
    }

    std::vector<uint8_t> rgba;
    bool gotRealPixels = false;
    Renderer* renderer = wv->renderer();
    if (renderer) {
        uint32_t capW = 0, capH = 0;
        std::vector<uint8_t> fb;
        if (renderer->captureScreenshotRGBA(fb, capW, capH)) {
            w = capW;
            h = capH;
            rgba.swap(fb);
            gotRealPixels = true;
        }
    }
    if (!gotRealPixels) {
        rgba.assign((size_t)w * h * 4, 0);
    }

    outW = w;
    outH = h;
    return ImageEncoder::encodePNG(rgba.data(), w, h,
                                   ImageEncoder::ImageColorSpace::RGBA);
}

// Heap payload carried by the repetitive screencast Timer. Plain malloc (not
// GC); freed only when the timer is removed (stopScreencast) -- a repetitive
// timer reuses the same data each fire, so the handler does NOT delete it.
struct ScreencastTimerData {
    CDPDispatcher* dispatcher;
    std::string sessionId;
};

static void buildFrame(CDPSession* s, WebView* wv, rapidjson::Value& frame,
                       rapidjson::Document::AllocatorType& alloc)
{
    std::string url = currentUrl(wv);
    std::string origin = currentOrigin(wv);
    frame.SetObject();
    frame.AddMember(
        "id", rapidjson::Value(s->frameId.c_str(), s->frameId.size(), alloc),
        alloc);
    frame.AddMember(
        "loaderId",
        rapidjson::Value(s->loaderId.c_str(), s->loaderId.size(), alloc),
        alloc);
    frame.AddMember("url", rapidjson::Value(url.c_str(), url.size(), alloc),
                    alloc);
    frame.AddMember("domainAndRegistry", "", alloc);
    frame.AddMember("securityOrigin",
                    rapidjson::Value(origin.c_str(), origin.size(), alloc),
                    alloc);
    frame.AddMember("mimeType", "text/html", alloc);
    frame.AddMember("secureContextType", "InsecureScheme", alloc);
    frame.AddMember("crossOriginIsolatedContextType", "NotIsolated", alloc);
    rapidjson::Value feats(rapidjson::kArrayType);
    frame.AddMember("gatedAPIFeatures", feats, alloc);
}

void PageDomain::emitLifecycle(const std::string& sessionId, const char* name)
{
    CDPSession* s = m_dispatcher->session();
    if (!s->lifecycleEventsEnabled) {
        return;
    }
    rapidjson::Document doc;
    rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
    rapidjson::Value params(rapidjson::kObjectType);
    params.AddMember(
        "frameId",
        rapidjson::Value(s->frameId.c_str(), s->frameId.size(), alloc), alloc);
    params.AddMember(
        "loaderId",
        rapidjson::Value(s->loaderId.c_str(), s->loaderId.size(), alloc),
        alloc);
    params.AddMember("name", rapidjson::Value(name, alloc), alloc);
    params.AddMember("timestamp", (double)longTickCount(), alloc);
    CDPCommand evt(m_dispatcher, Optional<int64_t>(), sessionId, nullptr);
    evt.sendEvent("Page.lifecycleEvent", params, doc);
}

void PageDomain::emitExecutionContextCreated(const std::string& sessionId)
{
    CDPSession* s = m_dispatcher->session();
    WebView* wv = m_dispatcher->webView();
    std::string origin = currentOrigin(wv);

    rapidjson::Document doc;
    rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
    rapidjson::Value params(rapidjson::kObjectType);
    rapidjson::Value context(rapidjson::kObjectType);
    context.AddMember("id", (int)s->executionContextId, alloc);
    context.AddMember("origin",
                      rapidjson::Value(origin.c_str(), origin.size(), alloc),
                      alloc);
    context.AddMember("name", "", alloc);
    context.AddMember("uniqueId", "1", alloc);
    rapidjson::Value auxData(rapidjson::kObjectType);
    auxData.AddMember("isDefault", true, alloc);
    auxData.AddMember("type", "default", alloc);
    auxData.AddMember(
        "frameId",
        rapidjson::Value(s->frameId.c_str(), s->frameId.size(), alloc), alloc);
    context.AddMember("auxData", auxData, alloc);
    params.AddMember("context", context, alloc);
    CDPCommand evt(m_dispatcher, Optional<int64_t>(), sessionId, nullptr);
    evt.sendEvent("Runtime.executionContextCreated", params, doc);
}

void PageDomain::emitIsolatedWorldCreated(const std::string& sessionId,
                                          uint32_t contextId,
                                          const std::string& worldName,
                                          const std::string& uniqueId,
                                          const std::string& frameId)
{
    CDPSession* s = m_dispatcher->session();
    WebView* wv = m_dispatcher->webView();
    std::string origin = currentOrigin(wv);

    rapidjson::Document doc;
    rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
    rapidjson::Value params(rapidjson::kObjectType);
    rapidjson::Value context(rapidjson::kObjectType);
    context.AddMember("id", (int)contextId, alloc);
    context.AddMember("origin",
                      rapidjson::Value(origin.c_str(), origin.size(), alloc),
                      alloc);
    context.AddMember(
        "name", rapidjson::Value(worldName.c_str(), worldName.size(), alloc),
        alloc);
    context.AddMember(
        "uniqueId", rapidjson::Value(uniqueId.c_str(), uniqueId.size(), alloc),
        alloc);
    rapidjson::Value auxData(rapidjson::kObjectType);
    auxData.AddMember("isDefault", false, alloc);
    auxData.AddMember("type", "isolated", alloc);
    auxData.AddMember("frameId",
                      rapidjson::Value(frameId.c_str(), frameId.size(), alloc),
                      alloc);
    context.AddMember("auxData", auxData, alloc);
    params.AddMember("context", context, alloc);
    CDPCommand evt(m_dispatcher, Optional<int64_t>(), sessionId, nullptr);
    evt.sendEvent("Runtime.executionContextCreated", params, doc);
}

// A BrowsingContext discovered in the frame tree, with its ordinal path (the
// stable key, see ChildFrame::path) and the path of its immediate parent frame
// (empty == the main frame).
struct DiscoveredFrame {
    BrowsingContext* bc;
    std::vector<uint32_t> path;
    std::vector<uint32_t> parentPath;
};

// Recursively collect every descendant iframe BrowsingContext of `bc` in
// document order. `bc` lives at `bcPath` (empty for the main frame). Each
// direct child gets path = bcPath + childIndex; the walk then recurses into
// that child so grandchild iframes are discovered too. Only contexts that have
// loaded a document are returned. iterateChildContext only yields a frame's own
// direct child iframes (it traverses that frame's document), so the recursion
// visits the whole tree.
static void collectDescendantContexts(BrowsingContext* bc,
                                      const std::vector<uint32_t>& bcPath,
                                      std::vector<DiscoveredFrame>& out)
{
    if (!bc) {
        return;
    }
    uint32_t idx = 0;
    bc->iterateChildContext([&](BrowsingContext* child) {
        if (child && child->document()) {
            DiscoveredFrame df;
            df.bc = child;
            df.path = bcPath;
            df.path.push_back(idx);
            df.parentPath = bcPath;
            out.push_back(df);
            collectDescendantContexts(child, df.path, out);
        }
        idx++;
    });
}

// Collect every iframe BrowsingContext in the tree (direct children and deeper
// descendants), pre-order, keyed by ordinal path.
static void collectChildContexts(WebView* wv, std::vector<DiscoveredFrame>& out)
{
    BrowsingContext* main = wv->mainBrowsingContext();
    if (!main) {
        return;
    }
    collectDescendantContexts(main, std::vector<uint32_t>(), out);
}

// Re-resolve a frame's live BrowsingContext by following its ordinal path from
// the main frame. Returns nullptr if any step no longer exists (the frame was
// detached).
static BrowsingContext* resolveByPath(WebView* wv,
                                      const std::vector<uint32_t>& path)
{
    BrowsingContext* bc = wv->mainBrowsingContext();
    for (uint32_t want : path) {
        if (!bc) {
            return nullptr;
        }
        std::vector<BrowsingContext*> kids;
        bc->iterateChildContext([&kids](BrowsingContext* child) {
            kids.push_back(child); // keep index alignment incl. unloaded
        });
        if (want >= kids.size() || !kids[want] || !kids[want]->document()) {
            return nullptr;
        }
        bc = kids[want];
    }
    return bc;
}

static std::string contextUrl(BrowsingContext* bc)
{
    if (bc && bc->document() && bc->document()->urlString()) {
        return bc->document()->urlString()->toUTF8NonGCString();
    }
    return "about:blank";
}

static std::string contextOrigin(BrowsingContext* bc)
{
    if (bc && bc->document() && bc->document()->origin()) {
        return bc->document()->origin()->toUTF8NonGCString();
    }
    return "";
}

// Look up a frameId for a given ordinal path: the main frame's id for an empty
// path, otherwise the matching ChildFrame's id (or "" if not yet recorded).
static std::string frameIdForPath(CDPSession* s,
                                  const std::vector<uint32_t>& path)
{
    if (path.empty()) {
        return s->frameId;
    }
    for (const ChildFrame& cf : s->childFrames) {
        if (cf.path == path) {
            return cf.frameId;
        }
    }
    return "";
}

void PageDomain::ensureChildFrameRecords()
{
    CDPSession* s = m_dispatcher->session();
    WebView* wv = m_dispatcher->webView();

    std::vector<DiscoveredFrame> frames;
    collectChildContexts(wv, frames);

    // Pre-order, so a frame's parent record already exists when we reach it.
    for (const DiscoveredFrame& df : frames) {
        bool have = false;
        for (const ChildFrame& existing : s->childFrames) {
            if (existing.path == df.path) {
                have = true;
                break;
            }
        }
        if (!have) {
            ChildFrame nf;
            nf.path = df.path;
            nf.contextId = s->nextChildContextId++;
            nf.frameId =
                "FID-CHILD-" + std::to_string(++s->childFrameCounter) + "00000";
            nf.parentFrameId = frameIdForPath(s, df.parentPath);
            nf.attachEmitted = false;
            s->childFrames.push_back(nf);
        }
    }
}

// Emit Page.frameDetached for any recorded child frame whose BrowsingContext is
// no longer reachable in the live tree (its iframe was removed, or an
// ancestor's was). Removes the dropped frames' records and their child-bound
// isolated worlds. Detaching a parent implicitly detaches its descendants, so
// each gone frame is reported (Chrome emits frameDetached per removed frame).
void PageDomain::sweepDetachedFrames(const std::string& sessionId)
{
    CDPSession* s = m_dispatcher->session();
    WebView* wv = m_dispatcher->webView();

    std::vector<ChildFrame> kept;
    std::vector<std::string> detachedIds;
    for (const ChildFrame& cf : s->childFrames) {
        if (resolveByPath(wv, cf.path)) {
            kept.push_back(cf);
        } else {
            detachedIds.push_back(cf.frameId);
        }
    }
    if (detachedIds.empty()) {
        return;
    }
    s->childFrames.swap(kept);

    // Drop isolated worlds bound to a detached child frame.
    {
        std::vector<IsolatedWorld> keptWorlds;
        for (const IsolatedWorld& w : s->isolatedWorlds) {
            bool gone = false;
            if (w.isChild) {
                for (const std::string& id : detachedIds) {
                    if (w.frameId == id) {
                        gone = true;
                        break;
                    }
                }
            }
            if (!gone) {
                keptWorlds.push_back(w);
            }
        }
        s->isolatedWorlds.swap(keptWorlds);
    }

    if (!s->pageEnabled) {
        return;
    }
    for (const std::string& id : detachedIds) {
        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value params(rapidjson::kObjectType);
        params.AddMember("frameId",
                         rapidjson::Value(id.c_str(), id.size(), alloc), alloc);
        params.AddMember("reason", "remove", alloc);
        CDPCommand evt(m_dispatcher, Optional<int64_t>(), sessionId, nullptr);
        evt.sendEvent("Page.frameDetached", params, doc);
    }
}

void PageDomain::discoverChildFrames(const std::string& sessionId)
{
    CDPSession* s = m_dispatcher->session();
    WebView* wv = m_dispatcher->webView();

    // Reconcile records against the live tree first: prune + emit
    // frameDetached for frames whose iframe was removed, then (re)create
    // records for any newly attached frames.
    sweepDetachedFrames(sessionId);
    ensureChildFrameRecords();

    std::vector<DiscoveredFrame> frames;
    collectChildContexts(wv, frames);

    for (const DiscoveredFrame& df : frames) {
        BrowsingContext* child = df.bc;

        ChildFrame* cf = nullptr;
        for (ChildFrame& existing : s->childFrames) {
            if (existing.path == df.path) {
                cf = &existing;
                break;
            }
        }
        if (!cf) {
            continue;
        }

        if (cf->attachEmitted) {
            continue;
        }
        cf->attachEmitted = true;

        std::string childUrl = contextUrl(child);
        std::string childOrigin = contextOrigin(child);
        const std::string& parentFrameId = cf->parentFrameId;

        // Page.frameAttached: parentFrameId is the immediate parent frame
        // (the main frame for a direct child, the enclosing iframe's frame for
        // a grandchild).
        if (s->pageEnabled) {
            rapidjson::Document doc;
            rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
            rapidjson::Value params(rapidjson::kObjectType);
            params.AddMember("frameId",
                             rapidjson::Value(cf->frameId.c_str(),
                                              cf->frameId.size(), alloc),
                             alloc);
            params.AddMember("parentFrameId",
                             rapidjson::Value(parentFrameId.c_str(),
                                              parentFrameId.size(), alloc),
                             alloc);
            CDPCommand evt(m_dispatcher, Optional<int64_t>(), sessionId,
                           nullptr);
            evt.sendEvent("Page.frameAttached", params, doc);

            // Page.frameNavigated for the child's loaded document.
            rapidjson::Document ndoc;
            rapidjson::Document::AllocatorType& nalloc = ndoc.GetAllocator();
            rapidjson::Value nparams(rapidjson::kObjectType);
            rapidjson::Value frame(rapidjson::kObjectType);
            frame.AddMember("id",
                            rapidjson::Value(cf->frameId.c_str(),
                                             cf->frameId.size(), nalloc),
                            nalloc);
            frame.AddMember("parentId",
                            rapidjson::Value(parentFrameId.c_str(),
                                             parentFrameId.size(), nalloc),
                            nalloc);
            frame.AddMember("loaderId",
                            rapidjson::Value(s->loaderId.c_str(),
                                             s->loaderId.size(), nalloc),
                            nalloc);
            frame.AddMember(
                "url",
                rapidjson::Value(childUrl.c_str(), childUrl.size(), nalloc),
                nalloc);
            frame.AddMember("domainAndRegistry", "", nalloc);
            frame.AddMember("securityOrigin",
                            rapidjson::Value(childOrigin.c_str(),
                                             childOrigin.size(), nalloc),
                            nalloc);
            frame.AddMember("mimeType", "text/html", nalloc);
            frame.AddMember("secureContextType", "InsecureScheme", nalloc);
            frame.AddMember("crossOriginIsolatedContextType", "NotIsolated",
                            nalloc);
            rapidjson::Value feats(rapidjson::kArrayType);
            frame.AddMember("gatedAPIFeatures", feats, nalloc);
            nparams.AddMember("frame", frame, nalloc);
            CDPCommand nevt(m_dispatcher, Optional<int64_t>(), sessionId,
                            nullptr);
            nevt.sendEvent("Page.frameNavigated", nparams, ndoc);
        }

        // Runtime.executionContextCreated for the child frame's default world.
        if (s->runtimeEnabled) {
            rapidjson::Document doc;
            rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
            rapidjson::Value params(rapidjson::kObjectType);
            rapidjson::Value context(rapidjson::kObjectType);
            context.AddMember("id", (int)cf->contextId, alloc);
            context.AddMember("origin",
                              rapidjson::Value(childOrigin.c_str(),
                                               childOrigin.size(), alloc),
                              alloc);
            context.AddMember("name", "", alloc);
            std::string uniqueId = "child-" + std::to_string(cf->contextId);
            context.AddMember(
                "uniqueId",
                rapidjson::Value(uniqueId.c_str(), uniqueId.size(), alloc),
                alloc);
            rapidjson::Value auxData(rapidjson::kObjectType);
            auxData.AddMember("isDefault", true, alloc);
            auxData.AddMember("type", "default", alloc);
            auxData.AddMember("frameId",
                              rapidjson::Value(cf->frameId.c_str(),
                                               cf->frameId.size(), alloc),
                              alloc);
            context.AddMember("auxData", auxData, alloc);
            params.AddMember("context", context, alloc);
            CDPCommand evt(m_dispatcher, Optional<int64_t>(), sessionId,
                           nullptr);
            evt.sendEvent("Runtime.executionContextCreated", params, doc);

            // Auto-create named isolated worlds (registered via
            // addScriptToEvaluateOnNewDocument{worldName}, e.g. puppeteer's
            // utility world) bound to this new child frame, so per-frame
            // helpers (frame.$, $$, waitForSelector) resolve a context without
            // an explicit createIsolatedWorld for the child.
            for (const std::string& wn : s->newDocumentWorldNames) {
                uint32_t wctx = s->nextIsolatedContextId++;
                std::string wuid = "world-" + std::to_string(wctx);
                IsolatedWorld iw;
                iw.contextId = wctx;
                iw.worldName = wn;
                iw.uniqueId = wuid;
                iw.frameId = cf->frameId;
                iw.isChild = true;
                iw.childPath = cf->path;
                s->isolatedWorlds.push_back(iw);
                emitIsolatedWorldCreated(sessionId, wctx, wn, wuid,
                                         cf->frameId);
            }
        }
    }
}

BrowsingContext* PageDomain::browsingContextForExecutionContextId(
    uint32_t contextId)
{
    CDPSession* s = m_dispatcher->session();
    WebView* wv = m_dispatcher->webView();

    std::vector<uint32_t> path;
    bool found = false;
    // Child frame's default world.
    for (const ChildFrame& cf : s->childFrames) {
        if (cf.contextId == contextId) {
            path = cf.path;
            found = true;
            break;
        }
    }
    // Child frame's isolated (utility) world.
    if (!found) {
        for (const IsolatedWorld& w : s->isolatedWorlds) {
            if (w.contextId == contextId && w.isChild) {
                path = w.childPath;
                found = true;
                break;
            }
        }
    }
    if (!found) {
        return nullptr;
    }

    return resolveByPath(wv, path);
}

void PageDomain::emitScreencastFrame(WebView* wv, const std::string& sessionId,
                                     int frameNumber)
{
    uint32_t outW = 0, outH = 0;
    std::vector<uint8_t> png = captureViewportPng(wv, outW, outH);
    std::string b64 = cdpBase64Encode(png.data(), png.size());

    rapidjson::Document doc;
    rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
    rapidjson::Value params(rapidjson::kObjectType);
    params.AddMember("data", rapidjson::Value(b64.c_str(), b64.size(), alloc),
                     alloc);

    // ScreencastFrameMetadata: device size + scroll/scale. Single active page
    // with no pinch-zoom, so pageScaleFactor=1 and scrollOffset=0; deviceWidth/
    // deviceHeight report the captured framebuffer dimensions. timestamp is
    // seconds since epoch (CDP Network.TimeSinceEpoch); longTickCount() is in
    // microseconds.
    rapidjson::Value meta(rapidjson::kObjectType);
    meta.AddMember("offsetTop", 0.0, alloc);
    meta.AddMember("pageScaleFactor", 1.0, alloc);
    meta.AddMember("deviceWidth", (double)outW, alloc);
    meta.AddMember("deviceHeight", (double)outH, alloc);
    meta.AddMember("scrollOffsetX", 0.0, alloc);
    meta.AddMember("scrollOffsetY", 0.0, alloc);
    meta.AddMember("timestamp", (double)longTickCount() / 1000000.0, alloc);
    params.AddMember("metadata", meta, alloc);

    // sessionId here is the FRAME number the client echoes in
    // screencastFrameAck (distinct from the CDP target sessionId envelope).
    params.AddMember("sessionId", frameNumber, alloc);

    CDPCommand evt(m_dispatcher, Optional<int64_t>(), sessionId, nullptr);
    evt.sendEvent("Page.screencastFrame", params, doc);
}

void PageDomain::onScreencastTimer(void* data)
{
    ScreencastTimerData* td = static_cast<ScreencastTimerData*>(data);
    CDPDispatcher* dispatcher = td->dispatcher;
    std::string sessionId = td->sessionId;
    // Repetitive timer: do NOT delete td here; stopScreencast frees it.

    TargetContext* ctx = dispatcher->contextForSession(sessionId);
    if (!ctx || !ctx->session) {
        return;
    }
    CDPSession* s = ctx->session;
    if (!s->screencastActive) {
        return;
    }

    // Backpressure: wait for the previous frame's ack before emitting the next
    // one. The first frame (screencastAcked initialised true) goes immediately.
    if (!s->screencastAcked) {
        return;
    }
    s->screencastAcked = false;

    dispatcher->page()->emitScreencastFrame(ctx->webView, sessionId,
                                            ++s->screencastSessionId);
}

void PageDomain::stopScreencast(CDPSession* s, WebView* wv)
{
    if (s->screencastTimerId != SIZE_MAX) {
        if (wv && wv->timer()) {
            wv->timer()->removeTimer(s->screencastTimerId);
        }
        s->screencastTimerId = SIZE_MAX;
    }
    s->screencastActive = false;
    s->screencastAcked = true;
}

void PageDomain::beginNavigation(const std::string& sessionId,
                                 const std::string& url)
{
    CDPSession* s = m_dispatcher->session();
    const std::string& fid = s->frameId;

    // New document: drop the previous document's resource list (the wire
    // records for the old page are no longer part of this frame's tree).
    s->resources.clear();

    // Reset real in-flight networkidle tracking for the new document: zero the
    // counter, clear the per-navigation idle-emitted guards, and cancel any
    // pending debounce timer left from the previous load.
    m_dispatcher->network()->resetNetworkIdle(m_dispatcher->webView());

    // A new document gets a fresh loaderId. Puppeteer's LifecycleWatcher only
    // resolves a new-document navigation when frame._loaderId changes (set from
    // the lifecycleEvent name:"init") AND the requested lifecycle has fired.
    s->loaderId = "LID-" + std::to_string(++s->loaderCounter) + "000000";

    // frameStartedLoading marks _hasStartedLoading on the frame.
    if (s->pageEnabled) {
        rapidjson::Document d;
        rapidjson::Document::AllocatorType& a = d.GetAllocator();
        rapidjson::Value p(rapidjson::kObjectType);
        p.AddMember("frameId", rapidjson::Value(fid.c_str(), fid.size(), a), a);
        CDPCommand e(m_dispatcher, Optional<int64_t>(), sessionId, nullptr);
        e.sendEvent("Page.frameStartedLoading", p, d);
    }

    // lifecycleEvent name:"init" sets frame._loaderId to the new loaderId.
    emitLifecycle(sessionId, "init");

    // Network events for the top-level document load.
    //
    // For real HTTP(S) navigations (Fetch not intercepting), the document is
    // loaded through the ResourceLoader, whose CDP network hook emits the REAL
    // requestWillBeSent/responseReceived/loadingFinished (actual
    // status/headers/body). Emitting synthetic ones here too would duplicate
    // the document request, so we skip them.
    //
    // data: URLs (and other non-network schemes) have no real ResourceLoader
    // network request, so keep the synthetic triple for them.
    //
    // With Fetch interception active, the deferred synthetic path owns the
    // document events (request now, response on resume); the real hook
    // suppresses the document to avoid conflicting with that state machine.
    bool isDataUrl = url.compare(0, 5, "data:") == 0;
    bool isHttp =
        url.compare(0, 7, "http://") == 0 || url.compare(0, 8, "https://") == 0;
    if (s->fetchEnabled) {
        m_dispatcher->network()->emitNavigationRequest(sessionId, url);
    } else if (isHttp) {
        // Real hook emits the document events; nothing synthetic here.
    } else if (isDataUrl) {
        m_dispatcher->network()->emitNavigation(sessionId, url);
    } else {
        // Unknown scheme with no real network hook: keep synthetic.
        m_dispatcher->network()->emitNavigation(sessionId, url);
    }
}

void PageDomain::finishNavigation(const std::string& sessionId)
{
    CDPSession* s = m_dispatcher->session();
    WebView* wv = m_dispatcher->webView();
    const std::string& fid = s->frameId;

    // Page.addScriptToEvaluateOnNewDocument: evaluate registered scripts in the
    // new document's main world, in registration order. The HTML resource is
    // fetched deferred to the message loop, so at this point the new
    // Window/Document exist but the page's own inline scripts have NOT run yet
    // -- evaluating here injects before page script, matching puppeteer's
    // evaluateOnNewDocument semantics.
    for (const EvaluateOnNewDocumentScript& script :
         s->evaluateOnNewDocumentScripts) {
        wv->evaluateJavaScript(
            String::fromUTF8(script.source.data(), script.source.size()));
    }

    // Re-inject Runtime.addBinding bindings into the new document's main world
    // (the previous window[name] functions died with the old context).
    for (const std::string& b : s->bindings) {
        m_dispatcher->runtime()->injectBinding(wv, b);
    }

    // Cross-document navigation invalidates all node/object handles and the old
    // document's child iframe frames.
    m_dispatcher->nodeRegistry()->reset();
    m_dispatcher->remoteObjectStore()->reset();
    s->childFrames.clear();
    // Drop child-frame-bound isolated worlds (their frames are gone); keep
    // main-world ones, which finishNavigation re-announces below.
    {
        std::vector<IsolatedWorld> kept;
        for (const IsolatedWorld& w : s->isolatedWorlds) {
            if (!w.isChild) {
                kept.push_back(w);
            }
        }
        s->isolatedWorlds.swap(kept);
    }

    double ts = (double)longTickCount();

    if (s->pageEnabled) {
        rapidjson::Document fdoc;
        rapidjson::Document::AllocatorType& falloc = fdoc.GetAllocator();
        rapidjson::Value fparams(rapidjson::kObjectType);
        rapidjson::Value frame(rapidjson::kObjectType);
        buildFrame(s, wv, frame, falloc);
        fparams.AddMember("frame", frame, falloc);
        CDPCommand fevt(m_dispatcher, Optional<int64_t>(), sessionId, nullptr);
        fevt.sendEvent("Page.frameNavigated", fparams, fdoc);
    }

    if (s->domEnabled) {
        rapidjson::Document ddoc;
        rapidjson::Document::AllocatorType& dalloc = ddoc.GetAllocator();
        rapidjson::Value dparams(rapidjson::kObjectType);
        CDPCommand devt(m_dispatcher, Optional<int64_t>(), sessionId, nullptr);
        devt.sendEvent("DOM.documentUpdated", dparams, ddoc);
    }

    // The previous document's execution context is gone; advertise a new one so
    // that page.evaluate() after navigation runs against a live context.
    if (s->runtimeEnabled) {
        {
            rapidjson::Document rdoc;
            rapidjson::Document::AllocatorType& ralloc = rdoc.GetAllocator();
            rapidjson::Value rparams(rapidjson::kObjectType);
            CDPCommand revt(m_dispatcher, Optional<int64_t>(), sessionId,
                            nullptr);
            revt.sendEvent("Runtime.executionContextsCleared", rparams, rdoc);
        }
        emitExecutionContextCreated(sessionId);

        // Re-announce every isolated world (e.g. puppeteer's utility world).
        // executionContextsCleared invalidated their previous contexts; without
        // re-emitting them page.$ / $eval (which run in the utility world) wait
        // forever for a context that never reappears.
        for (const IsolatedWorld& w : s->isolatedWorlds) {
            emitIsolatedWorldCreated(sessionId, w.contextId, w.worldName,
                                     w.uniqueId, w.frameId);
        }
    }

    // Child iframes are created during layout/frame-tree build; ensure layout
    // has run so their BrowsingContexts (and contexts) exist, then announce
    // them via Page.frameAttached/frameNavigated +
    // Runtime.executionContextCreated.
    wv->layoutIfNeeded();
    discoverChildFrames(sessionId);

    // DOMContentLoaded + load lifecycle, loadEventFired, then
    // frameStoppedLoading.
    emitLifecycle(sessionId, "DOMContentLoaded");
    emitLifecycle(sessionId, "load");

    // networkAlmostIdle / networkIdle:
    //  - HTTP(S) documents load the document + subresources through the real
    //    ResourceLoader, whose CDP hook tracks in-flight requests and fires
    //    these lifecycle events after 500ms of network quiescence
    //    (NetworkDomain's debounce timer). Emitting them synchronously here
    //    would resolve networkidle0/2 before subresources finished, so we
    //    DON'T.
    //  - data:/about:/unknown schemes have no real network transport, so the
    //    in-flight tracker never fires; keep the synthetic immediate idle so
    //    page.goto(waitUntil:'networkidle0'/'networkidle2') still resolves.
    bool isHttpDocument = false;
    {
        BrowsingContext* bc = wv->mainBrowsingContext();
        if (bc && bc->document() && bc->document()->urlString()) {
            std::string u = bc->document()->urlString()->toUTF8NonGCString();
            isHttpDocument = u.compare(0, 7, "http://") == 0 ||
                             u.compare(0, 8, "https://") == 0;
        }
    }
    if (!isHttpDocument) {
        emitLifecycle(sessionId, "networkAlmostIdle");
        emitLifecycle(sessionId, "networkIdle");
    }

    if (s->pageEnabled) {
        rapidjson::Document ldoc;
        rapidjson::Document::AllocatorType& lalloc = ldoc.GetAllocator();
        rapidjson::Value lparams(rapidjson::kObjectType);
        lparams.AddMember("timestamp", ts, lalloc);
        CDPCommand levt(m_dispatcher, Optional<int64_t>(), sessionId, nullptr);
        levt.sendEvent("Page.loadEventFired", lparams, ldoc);

        rapidjson::Document sdoc;
        rapidjson::Document::AllocatorType& salloc = sdoc.GetAllocator();
        rapidjson::Value sparams(rapidjson::kObjectType);
        sparams.AddMember("frameId",
                          rapidjson::Value(fid.c_str(), fid.size(), salloc),
                          salloc);
        CDPCommand sevt(m_dispatcher, Optional<int64_t>(), sessionId, nullptr);
        sevt.sendEvent("Page.frameStoppedLoading", sparams, sdoc);
    }
}

void PageDomain::completeDeferredNavigation(const std::string& sessionId,
                                            const std::string& url)
{
    // beginNavigation already ran (loaderId bumped, frameStartedLoading, init
    // lifecycle, synthetic network triple emitted). Run the real load and the
    // rest of the lifecycle now. The routed context (m_current) is the one the
    // continue/fulfill command carried, so webView() is correct.
    WebView* wv = m_dispatcher->webView();
    wv->loadHTMLDocument(String::fromUTF8(url.data(), url.size()));
    // Now that the request has resumed, fire the deferred response phase.
    m_dispatcher->network()->emitNavigationResponse(sessionId, url);
    finishNavigation(sessionId);
}

void PageDomain::failDeferredNavigation(const std::string& sessionId,
                                        const std::string& url)
{
    CDPSession* s = m_dispatcher->session();

    // Network.loadingFailed for the navigation request so a Network-aware
    // client sees the failure. requestId matches the navigation request (==
    // loaderId).
    if (s->networkEnabled) {
        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value params(rapidjson::kObjectType);
        params.AddMember(
            "requestId",
            rapidjson::Value(s->loaderId.c_str(), s->loaderId.size(), alloc),
            alloc);
        params.AddMember("timestamp", (double)longTickCount(), alloc);
        params.AddMember("type", "Document", alloc);
        params.AddMember("errorText", "net::ERR_FAILED", alloc);
        params.AddMember("canceled", false, alloc);
        CDPCommand evt(m_dispatcher, Optional<int64_t>(), sessionId, nullptr);
        evt.sendEvent("Network.loadingFailed", params, doc);
    }

    // No document is loaded. Still emit frameStoppedLoading so a Page-aware
    // client's loading state settles.
    if (s->pageEnabled) {
        rapidjson::Document sdoc;
        rapidjson::Document::AllocatorType& salloc = sdoc.GetAllocator();
        rapidjson::Value sparams(rapidjson::kObjectType);
        sparams.AddMember(
            "frameId",
            rapidjson::Value(s->frameId.c_str(), s->frameId.size(), salloc),
            salloc);
        CDPCommand sevt(m_dispatcher, Optional<int64_t>(), sessionId, nullptr);
        sevt.sendEvent("Page.frameStoppedLoading", sparams, sdoc);
    }
}

void PageDomain::setDocumentContentFromHtml(const std::string& sessionId,
                                            const std::string& html)
{
    WebView* wv = m_dispatcher->webView();

    // Replace the document by loading the html as a data:text/html URL through
    // the same path navigate uses. data: URLs have no real network transport,
    // so beginNavigation emits the synthetic document triple and
    // finishNavigation emits the immediate networkidle -- exactly the
    // navigate-to-data: behaviour that page.goto("data:...") already relies on.
    std::string dataUrl = "data:text/html;charset=utf-8," + html;

    beginNavigation(sessionId, dataUrl);
    wv->loadHTMLDocument(String::fromUTF8(dataUrl.data(), dataUrl.size()));
    finishNavigation(sessionId);
}

void PageDomain::notifyDocumentRewritten(const std::string& sessionId)
{
    CDPSession* s = m_dispatcher->session();

    // The engine already swapped the document in place (document.open/write/
    // close ran synchronously during the Runtime.evaluate that triggered this).
    // Reproduce the pre-document CDP signalling that beginNavigation would do
    // -- minus the synthetic network triple, since a document rewrite is not a
    // network request -- so puppeteer's LifecycleWatcher observes a fresh
    // loader.

    // Reset real in-flight networkidle tracking for the new document.
    m_dispatcher->network()->resetNetworkIdle(m_dispatcher->webView());

    // New document => fresh loaderId (LifecycleWatcher keys "init" on the
    // loader changing, then resolves setContent once "load" fires for it).
    s->loaderId = "LID-" + std::to_string(++s->loaderCounter) + "000000";

    if (s->pageEnabled) {
        rapidjson::Document d;
        rapidjson::Document::AllocatorType& a = d.GetAllocator();
        rapidjson::Value p(rapidjson::kObjectType);
        p.AddMember("frameId",
                    rapidjson::Value(s->frameId.c_str(), s->frameId.size(), a),
                    a);
        CDPCommand e(m_dispatcher, Optional<int64_t>(), sessionId, nullptr);
        e.sendEvent("Page.frameStartedLoading", p, d);
    }

    // "init" sets puppeteer's frame._loaderId to the new loaderId and clears
    // the accumulated lifecycle set, so the subsequent DOMContentLoaded/load
    // (emitted by finishNavigation) complete the setContent watcher.
    emitLifecycle(sessionId, "init");

    finishNavigation(sessionId);
}

void PageDomain::processMessage(CDPCommand& cmd, const std::string& method)
{
    CDPSession* s = m_dispatcher->session();
    WebView* wv = m_dispatcher->webView();

    if (method == "enable") {
        s->pageEnabled = true;
        cmd.sendResultEmpty();
        return;
    }
    if (method == "disable") {
        s->pageEnabled = false;
        cmd.sendResultEmpty();
        return;
    }

    if (method == "setBypassCSP") {
        // Process-wide CSP enforcement bypass. When enabled, all CSP allow*
        // checks short-circuit to true (policies are still parsed, just not
        // enforced), matching Chrome's Page.setBypassCSP.
        bool enabled = cmd.params() && cmd.params()->HasMember("enabled") &&
                       (*cmd.params())["enabled"].IsBool() &&
                       (*cmd.params())["enabled"].GetBool();
        ContentSecurityPolicy::setBypass(enabled);
        cmd.sendResultEmpty();
        return;
    }

    if (method == "getFrameTree") {
        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        rapidjson::Value frameTree(rapidjson::kObjectType);

        if (cmd.sessionId() == "STARTUP") {
            // Graceful startup frame (Puppeteer probes before attach).
            rapidjson::Value frame(rapidjson::kObjectType);
            frame.AddMember("id", "TID-STARTUP", alloc);
            frame.AddMember("loaderId", "LID-STARTUP", alloc);
            frame.AddMember("securityOrigin", "chrome://newtab/", alloc);
            frame.AddMember("url", "about:blank", alloc);
            frame.AddMember("secureContextType", "Secure", alloc);
            frameTree.AddMember("frame", frame, alloc);
        } else {
            rapidjson::Value frame(rapidjson::kObjectType);
            buildFrame(s, wv, frame, alloc);
            frameTree.AddMember("frame", frame, alloc);

            // Include child iframe frames as a nested tree (direct children,
            // grandchildren, ...). Ensure records exist so the ids match those
            // used by the emitted frameAttached/executionContextCreated.
            sweepDetachedFrames(cmd.sessionId());
            ensureChildFrameRecords();
            std::vector<DiscoveredFrame> frames;
            collectChildContexts(wv, frames);

            // Recursively assemble the childFrames array for the frame at
            // `parentPath` (empty == main). Returns nullptr placeholder via the
            // out-param pattern: builds into `dst`.
            std::function<void(const std::vector<uint32_t>&, const std::string&,
                               rapidjson::Value&)>
                buildChildTree = [&](const std::vector<uint32_t>& parentPath,
                                     const std::string& parentFrameId,
                                     rapidjson::Value& dst) {
                    rapidjson::Value childFrames(rapidjson::kArrayType);
                    bool any = false;
                    for (const DiscoveredFrame& df : frames) {
                        if (df.parentPath != parentPath) {
                            continue;
                        }
                        ChildFrame* cf = nullptr;
                        for (ChildFrame& existing : s->childFrames) {
                            if (existing.path == df.path) {
                                cf = &existing;
                                break;
                            }
                        }
                        if (!cf) {
                            continue;
                        }
                        any = true;
                        std::string curl = contextUrl(df.bc);
                        std::string corigin = contextOrigin(df.bc);
                        rapidjson::Value childTree(rapidjson::kObjectType);
                        rapidjson::Value cframe(rapidjson::kObjectType);
                        cframe.AddMember("id",
                                         rapidjson::Value(cf->frameId.c_str(),
                                                          cf->frameId.size(),
                                                          alloc),
                                         alloc);
                        cframe.AddMember("parentId",
                                         rapidjson::Value(parentFrameId.c_str(),
                                                          parentFrameId.size(),
                                                          alloc),
                                         alloc);
                        cframe.AddMember("loaderId",
                                         rapidjson::Value(s->loaderId.c_str(),
                                                          s->loaderId.size(),
                                                          alloc),
                                         alloc);
                        cframe.AddMember(
                            "url",
                            rapidjson::Value(curl.c_str(), curl.size(), alloc),
                            alloc);
                        cframe.AddMember("domainAndRegistry", "", alloc);
                        cframe.AddMember("securityOrigin",
                                         rapidjson::Value(corigin.c_str(),
                                                          corigin.size(),
                                                          alloc),
                                         alloc);
                        cframe.AddMember("mimeType", "text/html", alloc);
                        cframe.AddMember("secureContextType", "InsecureScheme",
                                         alloc);
                        cframe.AddMember("crossOriginIsolatedContextType",
                                         "NotIsolated", alloc);
                        rapidjson::Value feats(rapidjson::kArrayType);
                        cframe.AddMember("gatedAPIFeatures", feats, alloc);
                        childTree.AddMember("frame", cframe, alloc);
                        // Recurse: this frame's own childFrames.
                        buildChildTree(df.path, cf->frameId, childTree);
                        childFrames.PushBack(childTree, alloc);
                    }
                    if (any) {
                        dst.AddMember("childFrames", childFrames, alloc);
                    }
                };
            buildChildTree(std::vector<uint32_t>(), s->frameId, frameTree);
        }
        result.AddMember("frameTree", frameTree, alloc);
        cmd.sendResult(result, doc);
        return;
    }

    if (method == "setLifecycleEventsEnabled") {
        bool enabled = false;
        if (cmd.params() && cmd.params()->HasMember("enabled") &&
            (*cmd.params())["enabled"].IsBool()) {
            enabled = (*cmd.params())["enabled"].GetBool();
        }
        s->lifecycleEventsEnabled = enabled;
        cmd.sendResultEmpty();
        return;
    }

    if (method == "navigate") {
        if (!cmd.params() || !cmd.params()->HasMember("url") ||
            !(*cmd.params())["url"].IsString()) {
            cmd.sendError(-32602, "'url' is required");
            return;
        }
        const char* url = (*cmd.params())["url"].GetString();
        const std::string& sid = cmd.sessionId();
        const std::string& fid = s->frameId;

        beginNavigation(sid, std::string(url));

        // Network.emulateNetworkConditions(offline) / setBlockedURLs: a blocked
        // top-level document fails the navigation. Emit the document request +
        // loadingFailed (with the proper net:: error) for Network-aware
        // clients, settle the frame loading state, and return errorText in the
        // navigate result -- puppeteer's navigate() turns a non-empty errorText
        // into a rejected page.goto(). Skip the real load and the success
        // lifecycle.
        std::string navBlockError;
        if (m_dispatcher->network()->shouldBlockUrl(s, std::string(url),
                                                    navBlockError)) {
            m_dispatcher->network()->emitNavigationRequest(sid,
                                                           std::string(url));
            m_dispatcher->network()->emitLoadingFailed(wv, s->loaderId,
                                                       navBlockError);
            if (s->pageEnabled) {
                rapidjson::Document sdoc;
                rapidjson::Document::AllocatorType& salloc =
                    sdoc.GetAllocator();
                rapidjson::Value sparams(rapidjson::kObjectType);
                sparams.AddMember(
                    "frameId",
                    rapidjson::Value(fid.c_str(), fid.size(), salloc), salloc);
                CDPCommand sevt(m_dispatcher, Optional<int64_t>(), sid,
                                nullptr);
                sevt.sendEvent("Page.frameStoppedLoading", sparams, sdoc);
            }
            rapidjson::Document doc;
            rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
            rapidjson::Value result(rapidjson::kObjectType);
            result.AddMember("frameId",
                             rapidjson::Value(fid.c_str(), fid.size(), alloc),
                             alloc);
            result.AddMember("loaderId",
                             rapidjson::Value(s->loaderId.c_str(),
                                              s->loaderId.size(), alloc),
                             alloc);
            result.AddMember("errorText",
                             rapidjson::Value(navBlockError.c_str(),
                                              navBlockError.size(), alloc),
                             alloc);
            cmd.sendResult(result, doc);
            return;
        }

        // ensureNewDocumentNavigation keys on a non-empty loaderId in the
        // navigate result; send it before the synthesized lifecycle, and (for
        // the Fetch path) before parking so puppeteer's goto promise proceeds.
        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        result.AddMember(
            "frameId", rapidjson::Value(fid.c_str(), fid.size(), alloc), alloc);
        result.AddMember(
            "loaderId",
            rapidjson::Value(s->loaderId.c_str(), s->loaderId.size(), alloc),
            alloc);
        cmd.sendResult(result, doc);

        // Request interception: emit Fetch.requestPaused and park the load. The
        // actual loadHTMLDocument + finishNavigation run later, from the
        // matching Fetch.continueRequest/fulfillRequest/failRequest (each its
        // own message-loop turn -- no blocking, no deadlock).
        //
        // data: URLs are excluded: they are not network requests, so neither
        // Chrome nor puppeteer treats them as interceptable -- puppeteer's
        // request.continue()/respond() are silent no-ops for data: URLs
        // (canBeIntercepted() returns false). Pausing one would hang goto()
        // forever (the client never sends continueRequest). For data: URLs the
        // inline path below still emits Network.requestWillBeSent, so the
        // page.on('request') listener fires; the request just cannot be
        // continued/fulfilled (a puppeteer-side limitation, not ours).
        bool isDataUrl = std::string(url).compare(0, 5, "data:") == 0;
        if (s->fetchEnabled && !isDataUrl) {
            std::string reqId = m_dispatcher->fetch()->emitNavigationPaused(
                sid, std::string(url));
            s->pendingFetchNav.active = true;
            s->pendingFetchNav.requestId = reqId;
            s->pendingFetchNav.url = std::string(url);
            s->pendingFetchNav.sessionId = sid;
            return;
        }

        // Perform the real navigation on the main thread (dispatchOnMain runs
        // here). loadHTMLDocument() is the same entry point the public
        // LoadURL() path uses (WebContainerImpl::LoadURL -> loadHTMLDocument).
        // It pushes a HistoryManager entry (HistoryManagerAction::Add).
        wv->loadHTMLDocument(String::fromUTF8(url, strlen(url)));

        // If Fetch is enabled (the inline fall-through reached only for data:
        // URLs), beginNavigation emitted just the request phase; emit the
        // deferred response phase now so the Network event triple is complete.
        if (s->fetchEnabled) {
            m_dispatcher->network()->emitNavigationResponse(sid,
                                                            std::string(url));
        }

        // MVP lifecycle: there is no WebView page-load completion hook to bind
        // to, so we synthesize a complete (reduced) lifecycle right after
        // issuing the navigation.
        finishNavigation(sid);
        return;
    }

    if (method == "setDocumentContent") {
        if (!cmd.params() || !cmd.params()->HasMember("html") ||
            !(*cmd.params())["html"].IsString()) {
            cmd.sendError(-32602, "'html' is required");
            return;
        }
        // frameId is accepted but only the main frame is supported: the engine
        // has no API to replace a child iframe's document out-of-band, so a
        // non-main frameId is treated as the main frame (best effort).
        std::string html = (*cmd.params())["html"].GetString();
        const std::string& sid = cmd.sessionId();

        setDocumentContentFromHtml(sid, html);

        cmd.sendResultEmpty();
        return;
    }

    if (method == "reload") {
        cmd.sendResultEmpty();
        return;
    }

    if (method == "bringToFront") {
        // Single active page per WebView and no windowing/visibility control is
        // exposed by the engine, so there is nothing to raise. Acknowledge so
        // page.bringToFront() resolves.
        cmd.sendResultEmpty();
        return;
    }

    if (method == "getNavigationHistory") {
        // Map the WebView's HistoryManager onto CDP's navigation history. The
        // entry id is the absolute index (stable within a getNavigationHistory
        // /navigateToHistoryEntry round-trip, which is all puppeteer needs for
        // goBack/goForward).
        HistoryManager* hm = wv->historyManager();

        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        result.AddMember("currentIndex", (int)hm->currentIndex(), alloc);
        rapidjson::Value entries(rapidjson::kArrayType);
        for (size_t i = 0; i < hm->entryCount(); i++) {
            String* u = hm->entryURL(i);
            std::string url = u ? u->toUTF8NonGCString() : std::string();
            String* t = hm->entryTitle(i);
            std::string title = t ? t->toUTF8NonGCString() : std::string();

            rapidjson::Value entry(rapidjson::kObjectType);
            entry.AddMember("id", (int)i, alloc);
            entry.AddMember(
                "url", rapidjson::Value(url.c_str(), url.size(), alloc), alloc);
            entry.AddMember("userTypedURL",
                            rapidjson::Value(url.c_str(), url.size(), alloc),
                            alloc);
            entry.AddMember(
                "title", rapidjson::Value(title.c_str(), title.size(), alloc),
                alloc);
            entry.AddMember("transitionType", "typed", alloc);
            entries.PushBack(entry, alloc);
        }
        result.AddMember("entries", entries, alloc);
        cmd.sendResult(result, doc);
        return;
    }

    if (method == "navigateToHistoryEntry") {
        if (!cmd.params() || !cmd.params()->HasMember("entryId") ||
            !(*cmd.params())["entryId"].IsInt()) {
            cmd.sendError(-32602, "'entryId' is required");
            return;
        }
        int entryId = (*cmd.params())["entryId"].GetInt();
        HistoryManager* hm = wv->historyManager();
        if (entryId < 0 || (size_t)entryId >= hm->entryCount()) {
            cmd.sendError(-32602, "Invalid 'entryId'");
            return;
        }

        const std::string& sid = cmd.sessionId();
        String* u = hm->entryURL((size_t)entryId);
        std::string url = u ? u->toUTF8NonGCString() : std::string();

        beginNavigation(sid, url);

        // Reuse HistoryManager navigation (navigateAsync -> navigate). This is
        // a real cross-document load to the target entry, going through the
        // same BrowsingContext rebuild that loadHTMLDocument triggers.
        hm->goToIndex((size_t)entryId);

        cmd.sendResultEmpty();

        // Synthesize the same lifecycle/context events as navigate so that
        // page.evaluate() / page.$ after goBack/goForward run against the new
        // document's freshly-announced contexts (incl. isolated worlds).
        finishNavigation(sid);
        return;
    }

    if (method == "resetNavigationHistory") {
        wv->historyManager()->clear();
        cmd.sendResultEmpty();
        return;
    }

    if (method == "getLayoutMetrics") {
        // Viewport size from the real Window (window.innerWidth/innerHeight),
        // falling back to the screen rect when there is no laid-out document.
        BrowsingContext* lbc = wv->mainBrowsingContext();
        Window* win = lbc ? lbc->window() : nullptr;
        int w = win ? (int)win->innerWidth() : 0;
        int h = win ? (int)win->innerHeight() : 0;
        if (w <= 0) {
            w = wv->screenInfo().rect.size().width();
        }
        if (h <= 0) {
            h = wv->screenInfo().rect.size().height();
        }

        // Content size from the root element's laid-out scroll extent
        // (documentElement.scrollWidth/scrollHeight), at least the viewport.
        int cw = w;
        int ch = h;
        if (lbc && lbc->document() && lbc->document()->documentElement()) {
            int sw = (int)lbc->document()->documentElement()->scrollWidth();
            int sh = (int)lbc->document()->documentElement()->scrollHeight();
            if (sw > cw) {
                cw = sw;
            }
            if (sh > ch) {
                ch = sh;
            }
        }

        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);

        rapidjson::Value layoutViewport(rapidjson::kObjectType);
        layoutViewport.AddMember("pageX", 0, alloc);
        layoutViewport.AddMember("pageY", 0, alloc);
        layoutViewport.AddMember("clientWidth", w, alloc);
        layoutViewport.AddMember("clientHeight", h, alloc);
        result.AddMember("layoutViewport", layoutViewport, alloc);

        rapidjson::Value visualViewport(rapidjson::kObjectType);
        visualViewport.AddMember("offsetX", 0, alloc);
        visualViewport.AddMember("offsetY", 0, alloc);
        visualViewport.AddMember("pageX", 0, alloc);
        visualViewport.AddMember("pageY", 0, alloc);
        visualViewport.AddMember("clientWidth", w, alloc);
        visualViewport.AddMember("clientHeight", h, alloc);
        visualViewport.AddMember("scale", 1, alloc);
        result.AddMember("visualViewport", visualViewport, alloc);

        rapidjson::Value contentSize(rapidjson::kObjectType);
        contentSize.AddMember("x", 0, alloc);
        contentSize.AddMember("y", 0, alloc);
        contentSize.AddMember("width", cw, alloc);
        contentSize.AddMember("height", ch, alloc);
        result.AddMember("contentSize", contentSize, alloc);

        rapidjson::Value cssLayoutViewport(rapidjson::kObjectType);
        cssLayoutViewport.AddMember("pageX", 0, alloc);
        cssLayoutViewport.AddMember("pageY", 0, alloc);
        cssLayoutViewport.AddMember("clientWidth", w, alloc);
        cssLayoutViewport.AddMember("clientHeight", h, alloc);
        result.AddMember("cssLayoutViewport", cssLayoutViewport, alloc);

        rapidjson::Value cssVisualViewport(rapidjson::kObjectType);
        cssVisualViewport.AddMember("offsetX", 0, alloc);
        cssVisualViewport.AddMember("offsetY", 0, alloc);
        cssVisualViewport.AddMember("pageX", 0, alloc);
        cssVisualViewport.AddMember("pageY", 0, alloc);
        cssVisualViewport.AddMember("clientWidth", w, alloc);
        cssVisualViewport.AddMember("clientHeight", h, alloc);
        cssVisualViewport.AddMember("scale", 1, alloc);
        result.AddMember("cssVisualViewport", cssVisualViewport, alloc);

        rapidjson::Value cssContentSize(rapidjson::kObjectType);
        cssContentSize.AddMember("x", 0, alloc);
        cssContentSize.AddMember("y", 0, alloc);
        cssContentSize.AddMember("width", cw, alloc);
        cssContentSize.AddMember("height", ch, alloc);
        result.AddMember("cssContentSize", cssContentSize, alloc);

        cmd.sendResult(result, doc);
        return;
    }

    if (method == "addScriptToEvaluateOnNewDocument") {
        std::string source;
        if (cmd.params() && cmd.params()->HasMember("source") &&
            (*cmd.params())["source"].IsString()) {
            source = (*cmd.params())["source"].GetString();
        }
        std::string identifier =
            "SCRIPT-" + std::to_string(++s->evaluateOnNewDocumentCounter);
        s->evaluateOnNewDocumentScripts.push_back({ identifier, source });

        // A worldName here registers a named isolated world that the browser
        // auto-creates for each new document (puppeteer's utility world).
        // Record it so child frames get an isolated-world context bound to
        // them.
        if (cmd.params() && cmd.params()->HasMember("worldName") &&
            (*cmd.params())["worldName"].IsString()) {
            std::string wn = (*cmd.params())["worldName"].GetString();
            if (!wn.empty()) {
                bool known = false;
                for (const std::string& n : s->newDocumentWorldNames) {
                    if (n == wn) {
                        known = true;
                        break;
                    }
                }
                if (!known) {
                    s->newDocumentWorldNames.push_back(wn);
                }
            }
        }

        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        result.AddMember(
            "identifier",
            rapidjson::Value(identifier.c_str(), identifier.size(), alloc),
            alloc);
        cmd.sendResult(result, doc);
        return;
    }

    if (method == "removeScriptToEvaluateOnNewDocument") {
        if (cmd.params() && cmd.params()->HasMember("identifier") &&
            (*cmd.params())["identifier"].IsString()) {
            std::string id = (*cmd.params())["identifier"].GetString();
            auto& v = s->evaluateOnNewDocumentScripts;
            for (auto it = v.begin(); it != v.end(); ++it) {
                if (it->identifier == id) {
                    v.erase(it);
                    break;
                }
            }
        }
        cmd.sendResultEmpty();
        return;
    }

    if (method == "captureScreenshot") {
        // format: png (default) | jpeg. quality: jpeg only. clip: optional
        // {x,y,width,height,scale}. Returns { data: <base64> }.
        std::string format = "png";
        if (cmd.params() && cmd.params()->HasMember("format") &&
            (*cmd.params())["format"].IsString()) {
            format = (*cmd.params())["format"].GetString();
        }

        // Viewport in device pixels.
        float dpr = wv->screenInfo().devicePixelRatio;
        if (dpr <= 0) {
            dpr = 1;
        }
        int viewW = wv->screenInfo().rect.size().width();
        int viewH = wv->screenInfo().rect.size().height();

        // Capture region (CSS px). Default: whole viewport.
        double clipX = 0, clipY = 0, clipW = viewW, clipH = viewH,
               clipScale = 1;
        if (cmd.params() && cmd.params()->HasMember("clip") &&
            (*cmd.params())["clip"].IsObject()) {
            const rapidjson::Value& clip = (*cmd.params())["clip"];
            if (clip.HasMember("x") && clip["x"].IsNumber())
                clipX = clip["x"].GetDouble();
            if (clip.HasMember("y") && clip["y"].IsNumber())
                clipY = clip["y"].GetDouble();
            if (clip.HasMember("width") && clip["width"].IsNumber())
                clipW = clip["width"].GetDouble();
            if (clip.HasMember("height") && clip["height"].IsNumber())
                clipH = clip["height"].GetDouble();
            if (clip.HasMember("scale") && clip["scale"].IsNumber())
                clipScale = clip["scale"].GetDouble();
        }
        if (clipScale <= 0) {
            clipScale = 1;
        }

        // Ensure layout is current so reported dimensions match the document.
        wv->layoutIfNeeded(false);

        // Output buffer dimensions in physical pixels.
        size_t outW = (size_t)(clipW * dpr * clipScale);
        size_t outH = (size_t)(clipH * dpr * clipScale);
        if (outW == 0) {
            outW = 1;
        }
        if (outH == 0) {
            outH = 1;
        }

        // Try to read back real pixels from the active renderer. The GL
        // backend implements captureScreenshotRGBA via glReadPixels; the Mock
        // (glib_headless) backend does not rasterize and returns false.
        std::vector<uint8_t> rgba;
        bool gotRealPixels = false;
        Renderer* renderer = wv->renderer();
        if (renderer) {
            // captureScreenshotRGBA paints + composites the current document
            // into the framebuffer and reads it back (GL backend).
            uint32_t capW = 0, capH = 0;
            std::vector<uint8_t> fb;
            if (renderer->captureScreenshotRGBA(fb, capW, capH)) {
                // Report the renderer's full framebuffer dimensions; clip is
                // not applied to the raw readback in this MVP.
                outW = capW;
                outH = capH;
                rgba.swap(fb);
                gotRealPixels = true;
            }
        }

        if (!gotRealPixels) {
            // Mock/headless backend: no real pixel buffer to read back, so the
            // captured surface is transparent. We still produce a valid,
            // correctly sized image so the CDP/puppeteer contract holds.
            rgba.assign((size_t)outW * outH * 4, 0);
        }

        // libjpeg is not linked in the glib_headless backend, so jpeg requests
        // fall back to PNG (the buffer is encoded the same way either way).
        (void)format;
        std::vector<uint8_t> encoded = ImageEncoder::encodePNG(
            rgba.data(), outW, outH, ImageEncoder::ImageColorSpace::RGBA);

        std::string b64 = cdpBase64Encode(encoded.data(), encoded.size());

        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        result.AddMember(
            "data", rapidjson::Value(b64.c_str(), b64.size(), alloc), alloc);
        cmd.sendResult(result, doc);
        return;
    }

    if (method == "startScreencast") {
        // Periodically capture the viewport and emit Page.screencastFrame.
        // format (png|jpeg), quality (jpeg), maxWidth/maxHeight, everyNthFrame
        // are accepted; only format/quality/everyNthFrame are stored. jpeg
        // falls back to PNG where libjpeg is absent (same as
        // captureScreenshot), and maxWidth/maxHeight are not applied to the raw
        // framebuffer readback in this MVP.
        if (cmd.params() && cmd.params()->HasMember("format") &&
            (*cmd.params())["format"].IsString()) {
            s->screencastFormat = (*cmd.params())["format"].GetString();
        }
        if (cmd.params() && cmd.params()->HasMember("quality") &&
            (*cmd.params())["quality"].IsInt()) {
            s->screencastQuality = (*cmd.params())["quality"].GetInt();
        }
        if (cmd.params() && cmd.params()->HasMember("everyNthFrame") &&
            (*cmd.params())["everyNthFrame"].IsInt()) {
            int n = (*cmd.params())["everyNthFrame"].GetInt();
            s->screencastEveryNthFrame = n > 0 ? n : 1;
        }

        // Re-arm cleanly if already running.
        stopScreencast(s, wv);

        GlobalScope* gs = wv->mainBrowsingContext()
                              ? wv->mainBrowsingContext()->window()
                              : nullptr;
        if (!wv->timer() || !gs) {
            // No message loop to drive captures; acknowledge so the client's
            // page.startScreencast() still resolves (no frames will arrive).
            cmd.sendResultEmpty();
            return;
        }

        s->screencastActive = true;
        s->screencastAcked = true; // first frame emits immediately

        // ~10 fps. The timer fires repetitively; backpressure in the handler
        // gates emission on the previous frame's ack.
        ScreencastTimerData* td = new ScreencastTimerData();
        td->dispatcher = m_dispatcher;
        td->sessionId = s->sessionId;
        s->screencastTimerId = wv->timer()->addTimer(
            100, gs, &PageDomain::onScreencastTimer, td, /*repetitive=*/true);

        cmd.sendResultEmpty();
        return;
    }

    if (method == "stopScreencast") {
        stopScreencast(s, wv);
        cmd.sendResultEmpty();
        return;
    }

    if (method == "screencastFrameAck") {
        // The client confirmed receipt of a frame; release backpressure so the
        // next timer tick emits the following frame. sessionId is the frame
        // number we sent; we accept it without strict matching (any ack
        // advances), which is sufficient for the single-in-flight model here.
        s->screencastAcked = true;
        cmd.sendResultEmpty();
        return;
    }

    if (method == "printToPDF") {
        // Honor the paper/layout options (paperWidth/Height, landscape, scale,
        // margins, pageRanges) by sizing the PDF pages and placing/scaling the
        // captured viewport bitmap inside each page's printable area, slicing
        // across multiple pages when the content overflows one page.
        // transferMode "ReturnAsStream" is supported below (data otherwise).
        PrintToPdfOptions opt = parsePrintToPdfOptions(cmd.params());

        // Full document content height (CSS px) drives pagination in both the
        // raster and fallback paths (this also forces a layout).
        double contentHCssPx = mainContentHeightCssPx(wv);

        std::vector<uint8_t> pdf;

#if defined(PORT_CANVAS_BACKEND_CAIRO)
        // Read back real pixels from the renderer (GL backend), then embed the
        // bitmap into a cairo PDF surface -- the same readback the screenshot
        // path uses, so content matches captureScreenshot.
        wv->layoutIfNeeded(false);
        Renderer* renderer = wv->renderer();
        if (renderer) {
            uint32_t capW = 0, capH = 0;
            std::vector<uint8_t> fb;
            if (renderer->captureScreenshotRGBA(fb, capW, capH)) {
                float dpr = wv->screenInfo().devicePixelRatio;
                pdf = buildRasterPdf(fb, capW, capH, dpr, contentHCssPx, opt);
            }
        }
#endif

        if (pdf.empty()) {
            // Mock/headless backend (no cairo, no real pixels), or any failure
            // above: emit structurally valid blank page(s) at the requested
            // paper size. Page count is derived from the document content
            // height (CSS px == pt at scale 1) vs one printable area so that
            // page-count verification reflects the pagination contract even
            // headless.
            double docHPts = contentHCssPx * opt.scale;
            int totalPages = 1;
            if (docHPts >= 1) {
                totalPages = (int)std::ceil(docHPts / opt.printableHeightPts());
                if (totalPages < 1) {
                    totalPages = 1;
                }
            }
            // Honor pageRanges: count only selected pages (matches the cairo
            // path's per-slice selection).
            int pageCount = 0;
            for (int p = 1; p <= totalPages; p++) {
                if (pageInRanges(opt.pageRanges, p, totalPages)) {
                    pageCount++;
                }
            }
            if (pageCount < 1) {
                pageCount = 1; // always emit at least one valid page
            }
            pdf = buildFallbackPdf(opt.pageWidthPts(), opt.pageHeightPts(),
                                   pageCount);
        }

        // transferMode "ReturnAsStream" (puppeteer's page.pdf() default):
        // return a stream handle; the client drains it via IO.read/IO.close.
        // Otherwise (default "ReturnAsBase64") return the data inline.
        std::string transferMode = "ReturnAsBase64";
        if (cmd.params() && cmd.params()->HasMember("transferMode") &&
            (*cmd.params())["transferMode"].IsString()) {
            transferMode = (*cmd.params())["transferMode"].GetString();
        }

        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);

        if (transferMode == "ReturnAsStream") {
            std::string handle =
                std::to_string(++s->ioStreamCounter); // IO handles are strings
            IOStream stream;
            stream.data = std::move(pdf);
            s->ioStreams[handle] = std::move(stream);
            result.AddMember(
                "stream",
                rapidjson::Value(handle.c_str(), handle.size(), alloc), alloc);
            // CDP also includes an empty data field in stream mode.
            result.AddMember("data", "", alloc);
        } else {
            std::string b64 = cdpBase64Encode(pdf.data(), pdf.size());
            result.AddMember("data",
                             rapidjson::Value(b64.c_str(), b64.size(), alloc),
                             alloc);
        }
        cmd.sendResult(result, doc);
        return;
    }

    if (method == "createIsolatedWorld") {
        // Allocate a distinct execution context id for the world and announce
        // it via Runtime.executionContextCreated so puppeteer's IsolatedWorld
        // resolves its context. Evaluations against this id route to the same
        // underlying Escargot context (no real isolation).
        uint32_t ctxId = s->nextIsolatedContextId++;
        std::string worldName;
        if (cmd.params() && cmd.params()->HasMember("worldName") &&
            (*cmd.params())["worldName"].IsString()) {
            worldName = (*cmd.params())["worldName"].GetString();
        }
        std::string uniqueId = "world-" + std::to_string(ctxId);

        // The world is created for a specific frame (puppeteer passes frameId).
        // If that frameId is a child iframe, bind the world to that frame so
        // its executionContextCreated carries the child frameId and evaluations
        // on this contextId route to the child's BrowsingContext.
        std::string frameId = s->frameId;
        bool isChild = false;
        std::vector<uint32_t> childPath;
        if (cmd.params() && cmd.params()->HasMember("frameId") &&
            (*cmd.params())["frameId"].IsString()) {
            std::string reqFrame = (*cmd.params())["frameId"].GetString();
            ensureChildFrameRecords();
            for (const ChildFrame& cf : s->childFrames) {
                if (cf.frameId == reqFrame) {
                    frameId = reqFrame;
                    isChild = true;
                    childPath = cf.path;
                    break;
                }
            }
        }

        // Record the world so it can be re-announced after each navigation
        // (puppeteer creates the utility world once and never re-calls
        // createIsolatedWorld; it expects the browser to re-emit the context).
        IsolatedWorld world;
        world.contextId = ctxId;
        world.worldName = worldName;
        world.uniqueId = uniqueId;
        world.frameId = frameId;
        world.isChild = isChild;
        world.childPath = childPath;
        s->isolatedWorlds.push_back(world);

        emitIsolatedWorldCreated(cmd.sessionId(), ctxId, worldName, uniqueId,
                                 frameId);

        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        result.AddMember("executionContextId", (int)ctxId, alloc);
        cmd.sendResult(result, doc);
        return;
    }

    if (method == "handleJavaScriptDialog") {
        // Single-thread MVP: alert/confirm/prompt do not block the engine; by
        // the time the client responds, the page has already resumed with the
        // default value (confirm=false, prompt=null). So this is an
        // acknowledgement only — accept/promptText cannot retroactively change
        // the value the page already received. We still emit
        // Page.javascriptDialogClosed so clients (puppeteer Dialog) settle.
        bool accept = false;
        if (cmd.params() && cmd.params()->HasMember("accept") &&
            (*cmd.params())["accept"].IsBool()) {
            accept = (*cmd.params())["accept"].GetBool();
        }
        std::string userInput;
        if (cmd.params() && cmd.params()->HasMember("promptText") &&
            (*cmd.params())["promptText"].IsString()) {
            userInput = (*cmd.params())["promptText"].GetString();
        }
        cmd.sendResultEmpty();

        if (s->pageEnabled) {
            rapidjson::Document doc;
            rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
            rapidjson::Value params(rapidjson::kObjectType);
            params.AddMember("result", accept, alloc);
            params.AddMember(
                "userInput",
                rapidjson::Value(userInput.c_str(), userInput.size(), alloc),
                alloc);
            CDPCommand evt(m_dispatcher, Optional<int64_t>(), cmd.sessionId(),
                           nullptr);
            evt.sendEvent("Page.javascriptDialogClosed", params, doc);
        }
        return;
    }

    if (method == "captureSnapshot") {
        // format: "mhtml" (only supported). Serializes the current main
        // document into a minimal MHTML (RFC 2557 multipart/related): a header
        // part plus the main HTML document, followed by one part per captured
        // subresource (text bodies inline, binary bodies base64). Returns {
        // data }.
        std::string format = "mhtml";
        if (cmd.params() && cmd.params()->HasMember("format") &&
            (*cmd.params())["format"].IsString()) {
            format = (*cmd.params())["format"].GetString();
        }
        if (format != "mhtml") {
            cmd.sendError(-32602, "Unsupported snapshot format");
            return;
        }

        wv->layoutIfNeeded(false);
        std::string mainUrl = currentUrl(wv);
        std::string html = mainDocumentHTML(wv);

        const std::string boundary = "----MultipartBoundary--starfish----";
        std::string mhtml;
        mhtml += "From: <Saved by Starfish>\r\n";
        mhtml += "Subject: \r\n";
        mhtml += "Date: \r\n";
        mhtml += "MIME-Version: 1.0\r\n";
        mhtml += "Content-Type: multipart/related;\r\n";
        mhtml += "\ttype=\"text/html\";\r\n";
        mhtml += "\tboundary=\"" + boundary + "\"\r\n";
        mhtml += "\r\n";

        // Main document part.
        mhtml += "--" + boundary + "\r\n";
        mhtml += "Content-Type: text/html\r\n";
        mhtml += "Content-Transfer-Encoding: quoted-printable\r\n";
        mhtml += "Content-Location: " + mainUrl + "\r\n";
        mhtml += "\r\n";
        // Emit the HTML verbatim (not strictly QP-encoded). Newlines normalized
        // to CRLF so the part body is well-formed for line-based MHTML readers.
        for (size_t i = 0; i < html.size(); i++) {
            char c = html[i];
            if (c == '\n') {
                mhtml += "\r\n";
            } else if (c == '\r') {
                // skip; a following '\n' (or standalone) emits CRLF
                if (i + 1 >= html.size() || html[i + 1] != '\n') {
                    mhtml += "\r\n";
                }
            } else {
                mhtml += c;
            }
        }
        mhtml += "\r\n";

        // Subresource parts: each recorded resource that has a captured body.
        for (const ResourceRecord& r : s->resources) {
            if (r.type == "Document") {
                continue; // the main document is already the first part
            }
            auto bit = s->networkBodies.find(r.requestId);
            if (bit == s->networkBodies.end() || bit->second.empty()) {
                continue;
            }
            const std::string& raw = bit->second;
            bool b64 = false;
            auto fit = s->networkBodyBase64.find(r.requestId);
            if (fit != s->networkBodyBase64.end()) {
                b64 = fit->second;
            }
            mhtml += "--" + boundary + "\r\n";
            mhtml += "Content-Type: " + r.mimeType + "\r\n";
            if (b64) {
                mhtml += "Content-Transfer-Encoding: base64\r\n";
            } else {
                mhtml += "Content-Transfer-Encoding: quoted-printable\r\n";
            }
            mhtml += "Content-Location: " + r.url + "\r\n";
            mhtml += "\r\n";
            if (b64) {
                mhtml += cdpBase64Encode(
                    reinterpret_cast<const uint8_t*>(raw.data()), raw.size());
                mhtml += "\r\n";
            } else {
                for (size_t i = 0; i < raw.size(); i++) {
                    char c = raw[i];
                    if (c == '\n') {
                        mhtml += "\r\n";
                    } else if (c == '\r') {
                        if (i + 1 >= raw.size() || raw[i + 1] != '\n') {
                            mhtml += "\r\n";
                        }
                    } else {
                        mhtml += c;
                    }
                }
                mhtml += "\r\n";
            }
        }

        // Closing boundary.
        mhtml += "--" + boundary + "--\r\n";

        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        result.AddMember("data",
                         rapidjson::Value(mhtml.c_str(), mhtml.size(), alloc),
                         alloc);
        cmd.sendResult(result, doc);
        return;
    }

    if (method == "getResourceTree") {
        // frameTree: { frame, resources[], childFrames? }. The main frame
        // reuses buildFrame; resources are the wire records seen by the network
        // hook (subresources + the document). Child frames are listed without
        // their own resource lists (single-frame resource tracking MVP).
        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        rapidjson::Value frameTree(rapidjson::kObjectType);

        rapidjson::Value frame(rapidjson::kObjectType);
        buildFrame(s, wv, frame, alloc);
        frameTree.AddMember("frame", frame, alloc);

        rapidjson::Value resources(rapidjson::kArrayType);
        for (const ResourceRecord& r : s->resources) {
            if (r.type == "Document") {
                continue; // the document is the frame itself, not a subresource
            }
            rapidjson::Value res(rapidjson::kObjectType);
            res.AddMember("url",
                          rapidjson::Value(r.url.c_str(), r.url.size(), alloc),
                          alloc);
            res.AddMember(
                "type", rapidjson::Value(r.type.c_str(), r.type.size(), alloc),
                alloc);
            res.AddMember(
                "mimeType",
                rapidjson::Value(r.mimeType.c_str(), r.mimeType.size(), alloc),
                alloc);
            resources.PushBack(res, alloc);
        }
        frameTree.AddMember("resources", resources, alloc);

        // Child iframe frames as a nested tree (no per-child resource list in
        // this MVP). Mirrors getFrameTree's nesting.
        ensureChildFrameRecords();
        std::vector<DiscoveredFrame> frames;
        collectChildContexts(wv, frames);

        std::function<void(const std::vector<uint32_t>&, const std::string&,
                           rapidjson::Value&)>
            buildResTree = [&](const std::vector<uint32_t>& parentPath,
                               const std::string& parentFrameId,
                               rapidjson::Value& dst) {
                rapidjson::Value childFrames(rapidjson::kArrayType);
                bool any = false;
                for (const DiscoveredFrame& df : frames) {
                    if (df.parentPath != parentPath) {
                        continue;
                    }
                    ChildFrame* cf = nullptr;
                    for (ChildFrame& existing : s->childFrames) {
                        if (existing.path == df.path) {
                            cf = &existing;
                            break;
                        }
                    }
                    if (!cf) {
                        continue;
                    }
                    any = true;
                    std::string curl = contextUrl(df.bc);
                    std::string corigin = contextOrigin(df.bc);
                    rapidjson::Value childTree(rapidjson::kObjectType);
                    rapidjson::Value cframe(rapidjson::kObjectType);
                    cframe.AddMember("id",
                                     rapidjson::Value(cf->frameId.c_str(),
                                                      cf->frameId.size(),
                                                      alloc),
                                     alloc);
                    cframe.AddMember("parentId",
                                     rapidjson::Value(parentFrameId.c_str(),
                                                      parentFrameId.size(),
                                                      alloc),
                                     alloc);
                    cframe.AddMember("loaderId",
                                     rapidjson::Value(s->loaderId.c_str(),
                                                      s->loaderId.size(),
                                                      alloc),
                                     alloc);
                    cframe.AddMember(
                        "url",
                        rapidjson::Value(curl.c_str(), curl.size(), alloc),
                        alloc);
                    cframe.AddMember("domainAndRegistry", "", alloc);
                    cframe.AddMember("securityOrigin",
                                     rapidjson::Value(corigin.c_str(),
                                                      corigin.size(), alloc),
                                     alloc);
                    cframe.AddMember("mimeType", "text/html", alloc);
                    cframe.AddMember("secureContextType", "InsecureScheme",
                                     alloc);
                    cframe.AddMember("crossOriginIsolatedContextType",
                                     "NotIsolated", alloc);
                    rapidjson::Value feats(rapidjson::kArrayType);
                    cframe.AddMember("gatedAPIFeatures", feats, alloc);
                    childTree.AddMember("frame", cframe, alloc);
                    rapidjson::Value cres(rapidjson::kArrayType);
                    childTree.AddMember("resources", cres, alloc);
                    buildResTree(df.path, cf->frameId, childTree);
                    childFrames.PushBack(childTree, alloc);
                }
                if (any) {
                    dst.AddMember("childFrames", childFrames, alloc);
                }
            };
        buildResTree(std::vector<uint32_t>(), s->frameId, frameTree);

        result.AddMember("frameTree", frameTree, alloc);
        cmd.sendResult(result, doc);
        return;
    }

    if (method == "getResourceContent") {
        // { frameId, url } -> { content, base64Encoded }. The main document URL
        // returns the live serialized HTML; any other URL returns its captured
        // network body (base64 for binary). frameId is accepted but only the
        // main frame's resources are tracked.
        std::string url;
        if (cmd.params() && cmd.params()->HasMember("url") &&
            (*cmd.params())["url"].IsString()) {
            url = (*cmd.params())["url"].GetString();
        }
        if (url.empty()) {
            cmd.sendError(-32602, "'url' is required");
            return;
        }

        std::string content;
        bool base64Encoded = false;
        bool found = false;

        // Main document: serialize live (matches the captureSnapshot main
        // part).
        if (url == currentUrl(wv)) {
            content = mainDocumentHTML(wv);
            found = !content.empty();
        }

        // Otherwise look up the captured body by URL (most recent match wins).
        if (!found) {
            for (auto it = s->resources.rbegin(); it != s->resources.rend();
                 ++it) {
                if (it->url != url) {
                    continue;
                }
                auto bit = s->networkBodies.find(it->requestId);
                if (bit == s->networkBodies.end()) {
                    continue;
                }
                bool b64 = false;
                auto fit = s->networkBodyBase64.find(it->requestId);
                if (fit != s->networkBodyBase64.end()) {
                    b64 = fit->second;
                }
                if (b64) {
                    content = cdpBase64Encode(
                        reinterpret_cast<const uint8_t*>(bit->second.data()),
                        bit->second.size());
                    base64Encoded = true;
                } else {
                    content = bit->second;
                }
                found = true;
                break;
            }
        }

        if (!found) {
            cmd.sendError(-32000, "No resource with given URL found");
            return;
        }

        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        result.AddMember(
            "content", rapidjson::Value(content.c_str(), content.size(), alloc),
            alloc);
        result.AddMember("base64Encoded", base64Encoded, alloc);
        cmd.sendResult(result, doc);
        return;
    }

    if (method == "getAppManifest") {
        // { url, data?, errors[], parsed? }. url is the absolute URL of the
        // document's <link rel="manifest">. If a body for that URL was captured
        // by the network hook, it is returned as `data`. Manifest parsing is
        // not implemented, so `parsed` is omitted and `errors` is always empty.
        std::string url = manifestUrl(wv);

        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        result.AddMember(
            "url", rapidjson::Value(url.c_str(), url.size(), alloc), alloc);

        // Attach the manifest body if it was captured for this URL.
        if (!url.empty()) {
            for (auto it = s->resources.rbegin(); it != s->resources.rend();
                 ++it) {
                if (it->url != url) {
                    continue;
                }
                auto bit = s->networkBodies.find(it->requestId);
                if (bit == s->networkBodies.end()) {
                    continue;
                }
                auto fit = s->networkBodyBase64.find(it->requestId);
                bool b64 = fit != s->networkBodyBase64.end() && fit->second;
                if (!b64) {
                    result.AddMember("data",
                                     rapidjson::Value(bit->second.c_str(),
                                                      bit->second.size(),
                                                      alloc),
                                     alloc);
                }
                break;
            }
        }

        rapidjson::Value errors(rapidjson::kArrayType);
        result.AddMember("errors", errors, alloc);
        cmd.sendResult(result, doc);
        return;
    }

    if (method == "getInstallabilityErrors") {
        // PWA installability is not evaluated; report no errors.
        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        rapidjson::Value errs(rapidjson::kArrayType);
        result.AddMember("installabilityErrors", errs, alloc);
        cmd.sendResult(result, doc);
        return;
    }

    if (method == "getManifestIcons") {
        // No manifest icon extraction; ack with an empty result (primaryIcon
        // omitted).
        cmd.sendResultEmpty();
        return;
    }

    if (method == "generateTestReport") {
        // The engine has no ReportingObserver implementation, so no report can
        // be delivered to page script. Accept the call so tooling does not
        // error.
        cmd.sendResultEmpty();
        return;
    }

    if (method == "setInterceptFileChooserDialog") {
        // Headless: there is no native file chooser to intercept. Record the
        // requested state and ack; no Page.fileChooserOpened is ever emitted.
        s->interceptFileChooserDialog = cmd.params() &&
                                        cmd.params()->HasMember("enabled") &&
                                        (*cmd.params())["enabled"].IsBool() &&
                                        (*cmd.params())["enabled"].GetBool();
        cmd.sendResultEmpty();
        return;
    }

    if (method == "crash") {
        // Intentionally not crashing the process; ack so the client's request
        // completes. (A real crash would tear down the inspector connection.)
        cmd.sendResultEmpty();
        return;
    }

    cmd.sendError(-32601, "'method' wasn't found");
}

} // namespace Starfish

#endif
