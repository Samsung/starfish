# Specification

## HTML
This section describes the complete list of supported HTML tags and attributes by the Web widget engine. Please note that only the tags and attributes mentioned explicitly in this section are supported. In addition, the Widget engine supports only HTML5 documents, and it assumes all input documents are HTML5 documents even if `!DOCTYPE` is not explicitly specified.

| HTML Tag | Attribute | Allowed Value | Usage | Note |
|----------|-----------|---------------|-------|------|
| [Global Attribute](https://www.w3.org/TR/html5/dom.html#global-attributes) | class | &lt;string&gt; | &lt;element class="classname"&gt; |  |
|  | dir | ltr &#124; rtl | &lt;element dir="ltr"&gt; |  |
|  | id | &lt;string&gt; | &lt;element id="id"&gt; |  |
|  | style | &lt;css_styles&gt; | &lt;element style="css_styles"&gt; | &lt;css_styles&gt; must conform to the CSS section of this specification document. |
|  [html](https://www.w3.org/TR/html5/semantics.html#the-root-element)  |  |  | &lt;html&gt;&lt;/html&gt; |  |
|  [head](https://www.w3.org/TR/html5/document-metadata.html#the-head-element)  |  |  | &lt;head&gt;SAMSUNG&lt;/head&gt; |  |
|  [link](https://www.w3.org/TR/html5/document-metadata.html#the-link-element)  | rel | stylesheet | &lt;link rel="stylesheet"&gt; |  |
|  | href | &lt;URL&gt; | &lt;link rel="stylesheet" href="mystyle.css"&gt; |  |
|  | media | media query | &lt;link rel="stylesheet" href="mystyle.css" media="screen"&gt; |  |
|  | type | text/css | &lt;link rel="stylesheet" href="mystyle.css" type="text/css"&gt; |  |
|  [meta](https://www.w3.org/TR/html5/document-metadata.html#the-meta-element)  | charset | UTF-8 | &lt;meta charset="UTF-8"&gt; | Only UTF-8 is supported  |
|  | name | tizen-widget-transparent-background | &lt;meta name="tizen-widget-transparent-background" content="yes"&gt; | name and content are used to set the widget background transparent only. To do so, both name and content must be set in the same meta tag |
|  | content | yes &#124; no | &lt;meta name="tizen-widget-transparent-background" content="yes"&gt; |  |
|  [style](https://www.w3.org/TR/html5/document-metadata.html#the-style-element)  | media | media query | &lt;style type="text/css" media="screen"&gt;&lt;/style&gt; | |
|  | type | text/css | &lt;style type="text/css"&gt;&lt;/style&gt; | Only "text/css" type is supported. |
|  [body](https://www.w3.org/TR/html5/sections.html#the-body-element)  |  |  | &lt;body&gt;SAMSUNG&lt;/body&gt; |  |
|  [h1, h2, h3, h4, h5, and h6](https://www.w3.org/TR/html5/sections.html#the-h1,-h2,-h3,-h4,-h5,-and-h6-elements)  |  |  | &lt;h1&gt;&lt;\h1&gt;, &lt;h2&gt;&lt;\h2&gt;, &lt;h3&gt;&lt;\h3&gt;, etc. |  |
|  [p](https://www.w3.org/TR/html5/grouping-content.html#the-p-element)  |  |  | &lt;p&gt; |  |
|  [div](https://www.w3.org/TR/html5/grouping-content.html#the-div-element)  |  |  | &lt;div&gt;SAMSUNG&lt;/div&gt; |  |
|  [span](https://www.w3.org/TR/html5/text-level-semantics.html#the-span-element)  |  |  | &lt;span&gt;SAMSUNG&lt;/span&gt; |  |
|  [br](https://www.w3.org/TR/html5/text-level-semantics.html#the-br-element)  |  |  | &lt;br&gt; |  |
|  [image](https://www.w3.org/TR/html5/embedded-content-0.html#the-img-element)  | src | &lt;URL&gt; | &lt;img src="URL"&gt; |  Supported images are of type .png, .jpg, and .bmp |
|  | height | pixels | &lt;img height="pixels"&gt; |  |
|  | width | pixels | &lt;img width="pixels"&gt; |  |
|  [script](https://www.w3.org/TR/html5/scripting-1.html#the-script-element)  | src | &lt;URL&gt; | &lt;script src="URL"&gt;&lt;/script&gt; |  |
|  | type | text/javascript | &lt;script type="text/javascript"&gt;&lt;/script&gt; |  |
|  | charset | UTF-8 | &lt;script charset="UTF-8"&gt;&lt;/script&gt; | Only UTF-8 is supported |
|  [table](https://www.w3.org/TR/html5/tabular-data.html#the-table-element)  | width | pixels &#124; &lt;percentage&gt; | &lt;table width="400"&gt;&lt;/table&gt; |  |
|  | bgcolor | &lt;color&gt; | &lt;table bgcolor="blue"&gt;&lt;/table&gt; |  |
|  [caption](https://www.w3.org/TR/html5/tabular-data.html#the-caption-element) |  |  | &lt;caption&gt;SAMSUNG&lt;/caption&gt; |  |
|  [colgroup](https://www.w3.org/TR/html5/tabular-data.html#the-colgroup-element)  |  |  | &lt;colgroup&gt;&lt;/colgroup&gt; |  |
|  [tbody](https://www.w3.org/TR/html5/tabular-data.html#the-tbody-element)  |  |  | &lt;tbody&gt;&lt;/tbody&gt; |  |
|  [thead](https://www.w3.org/TR/html5/tabular-data.html#the-thead-element)  |  |  | &lt;thead&gt;&lt;/thead&gt; |  |
|  [tfoot](https://www.w3.org/TR/html5/tabular-data.html#the-tfoot-element)  |  |  | &lt;tfoot&gt;&lt;/tfoot&gt; |  |
|  [tr](https://www.w3.org/TR/html5/tabular-data.html#the-tr-element)  |  |  | &lt;tr&gt;&lt;/tr&gt; |  |
|  [td](https://www.w3.org/TR/html5/tabular-data.html#the-td-element), [th](https://www.w3.org/TR/html5/tabular-data.html#the-th-element) | width | pixels &#124; &lt;percentage&gt; | &lt;td width="30%"&gt;SAMSUNG&lt;/td&gt; |  |
|  | colspan | number | &lt;td colspan="2"&gt; |  |
|  | rowspan | number | &lt;td rowspan="2"&gt; |  |
|  | bgcolor | &lt;color&gt; | &lt;td bgcolor="blue"&gt;SAMSUNG&lt;/td&gt; |  |
|  [video](https://www.w3.org/TR/html5/embedded-content-0.html#the-video-element)  | src | &lt;URL&gt; | &lt;video src="movie.ogg" &gt; | [local&#124;network][absolute&#124;relative] URL |
|  | autoplay | autoplay | &lt;video width="320" height="240" autoplay&gt; |  |
|  | loop | loop | &lt;video loop&gt; |  |
|  | muted | muted | &lt;video muted&gt; |  |
|  | width | pixels | &lt;video width="320" height="240"&gt; |  |
|  | height | pixels | &lt;video width="320" height="240"&gt; |  |
|  [a](https://www.w3.org/TR/html5/text-level-semantics.html#the-a-element) | href | &lt;URL&gt; | &lt;a href="./content.html"&gt;content&lt;/a&gt; |  |
|  [pre](https://www.w3.org/TR/html5/grouping-content.html#the-pre-element) |  |  | &lt;pre&gt;SAMSUNG&lt;/pre&gt; |  |
|  [ul](https://www.w3.org/TR/html5/grouping-content.html#the-ul-element)  |  |  | &lt;ul&gt;&lt;/ul&gt; | List items are not displayed as bullets. |
|  [li](https://www.w3.org/TR/html5/grouping-content.html#the-li-element)  |  |  | &lt;li&gt;Coffee&lt;/li&gt; |  |
|  [audio](https://www.w3.org/TR/html5/embedded-content-0.html#the-audio-element) |  |  | &lt;audio&gt;&lt;/audio&gt; |  |
|  [source](https://www.w3.org/TR/html5/embedded-content-0.html#the-source-element) | src | &lt;URL&gt; | &lt;source src="horse.mp4"&gt; |  |
|  | type | MIME-type | &lt;source type="video/mp4"&gt; | Only video/mp4 and audio/mp4 are supported. |
|  [object](https://www.w3.org/TR/html5/embedded-content-0.html#the-object-element) |  |  | &lt;object&gt;&lt;/object&gt; |  |
|  [strong](https://www.w3.org/TR/html5/text-level-semantics.html#the-strong-element) |  |  | &lt;h1&gt;Chapter 1: &lt;strong&gt;The Praxis&lt;/strong&gt;&lt;/h1&gt; |  |
|  [DOCTYPE](https://www.w3.org/TR/html5/syntax.html#the-doctype)  |  | html | &lt;!DOCTYPE html&gt; | The DOCTYPE declaration must be the first tag in your HTML document. The Web widget engine supports HTML5 only. Other versions of HTMLs and HTML modes (such as quirks mode) are not supported.|

## DOM

| Interface            | Type   | Name                      | Description |
|----------------------|--------|---------------------------|-------------|
|[Common Definitions](https://heycam.github.io/webidl/#common) | typedef | (unsigned long long) DOMTimeStamp | The DOMTimeStamp type is used for representing a number of milliseconds, either as an absolute time (relative to some epoch) or as a relative amount of time. |
|  | typedef | (Int8Array or Int16Array or Int32Array or Uint8Array or Uint16Array or Uint32Array or Uint8ClampedArray or Float32Array or Float64Array or DataView) ArrayBufferView |  |
|  | typedef | (ArrayBufferView or ArrayBuffer) BufferSource |  |
|  | callback | Function = any (any... arguments) | |
|  | callback | VoidFunction = void () | |
| [Attr](https://dom.spec.whatwg.org/#interface-attr) | interface | Attr | Attr nodes are simply known as attributes. They are sometimes referred to as content attributes to avoid confusion with IDL attributes. |
|  | attribute | localName | Return the local name. |
|  | attribute | name | Return the qualified name. |
|  | attribute | value | Return the value. |
|  | attribute | ownerElement | Return context object’s element. |
|  | attribute | specified | Return true. |
| [CDATASection](https://dom.spec.whatwg.org/#interface-cdatasection) | interface | CDATASection |  |
| [CharacterData](https://dom.spec.whatwg.org/#interface-characterdata) | interface | CharacterData | CharacterData is an abstract interface and does not exist as node. It is used by Text, ProcessingInstruction, and Comment nodes. |
|  | attribute | data | Getter must return context object’s data. Its setter must replace data with node context object, offset 0, count context object’s length, and data new value. |
|  | attribute | length | Return context object’s length. |
| [ChildNode](https://dom.spec.whatwg.org/#childnode) | interface | ChildNode | The childNodes interface contains methods that are particular to Node objects that can have a parent. |
|  | method | void remove() | Removes this childNodes from the children list of its parent. |
| [Comment](https://dom.spec.whatwg.org/#interface-comment) | interface | Comment | The Comment interface represents textual notations within markup; although it is generally not visually shown, such comments are available to be read in the source view |
| | constructor | Comment(optional DOMString data = "") | Returns a Comment object with the parameter as its textual content. |
| [CSSRule](https://drafts.csswg.org/cssom/#the-cssrule-interface) | interface | CSSRule | The CSSRule interface represents an abstract, base CSS style rule. Each distinct CSS style rule type is represented by a distinct interface that inherits from this interface. |
|  | constant | STYLE_RULE = 1 |  |
|  | constant | CHARSET_RULE = 2 |  |
|  | constant | IMPORT_RULE = 3 |  |
|  | constant | MEDIA_RULE = 4 |  |
|  | constant | FONT_FACE_RULE = 5 |  |
|  | constant | PAGE_RULE = 6 |  |
|  | constant | MARGIN_RULE = 9 |  |
|  | constant | NAMESPACE_RULE = 10 |  |
|  | attribute | type | One of the Type constants indicating the type of CSS rule. |
|  | attribute | cssText | Returns a serialization of the CSS rule. |
|  | attribute | parentRule | Returns the parent CSS rule. |
|  | attribute | parentStyleSheet | Returns the parent CSS style sheet. |
| [CSSStyleDeclaration](https://dev.w3.org/csswg/cssom/#the-cssstyledeclaration-interface) | interface | CSSStyleDeclaration | The CSSStyleDeclaration interface represents a CSS declaration block, including its underlying state, where this underlying state depends upon the source of the CSSStyleDeclaration instance. |
|  | attribute | cssText | Returns the result of serializing the declarations, or sets cssText attribute if after parsing the given value, the return value is not null. |
|  | attribute | length | Returns the number of CSS declarations in the declarations. |
|  | method | getter DOMString item(unsigned long index) | Returns the property name of the CSS declaration at position index. |
|  | method | DOMString getPropertyValue(DOMString property) | Returns the property value |
|  | method | void setProperty(DOMString property, [TreatNullAs=EmptyString] DOMString value, [TreatNullAs=EmptyString] optional DOMString priority = "") | Sets the property |
|  | attribute | parentRule | Returns the parent CSS rule. |
| [CSSStyleRule](https://dev.w3.org/csswg/cssom/#the-cssstylerule-interface) | interface | CSSStyleRule | Represents a style rule. |
|  | attribute | selectorText | Returns the result of serializing the associated group of selectors. (Note: We will support result separated by ',' for a while.)|
|  | attribute | style | Returns a CSSStyleDeclaration object for the style rule. |
| [CSSImportRule](https://drafts.csswg.org/cssom/#the-cssimportrule-interface) | interface | CSSImportRule | Represents an @import at-rule. |
|  | attribute | href | Returns the URL specified by the @import at-rule. |
|  | attribute | styleSheet | Returns a CSS style sheet downloaded by @import at-rule. |
| [CSSGroupingRule](https://drafts.csswg.org/cssom/#the-cssgroupingrule-interface) | interface | CSSGroupingRule | Represents an at-rule that contains other rules nested inside itself. |
|  | attribute | cssRules | Returns a CSSRuleList object for the child CSS rules. |
|  | method | unsigned long insertRule(CSSOMString rule, optional unsigned long index = 0) | Returns the result of invoking insert a CSS rule rule into the child CSS rules at index. |
|  | method | void deleteRule(unsigned long index) | Removes a CSS rule from the child CSS rules at index. |
| [CSSConditionRule](https://drafts.csswg.org/css-conditional-3/#cssconditionrule) | interface | CSSConditionRule | Represents all the “conditional” at-rules, which consist of a condition and a statement block. |
|  | attribute | conditionText | Returns the result of serializing the associated condition. |
| [CSSMediaRule](https://drafts.csswg.org/css-conditional-3/#cssmediarule) | interface | CSSMediaRule | Represents a @media at-rule. |
|  | attribute | conditionText | Returns the value of media.mediaText on the rule. (CSSMediaRule-specific definition for attribute on CSSConditionRule) Note: Currently, widget engine supports only getter.|
| [CSSStyleSheet](https://drafts.csswg.org/cssom/#the-cssstylesheet-interface) | interface | CSSStyleSheet | Represents a CSS style sheet. |
| | attribute | ownerRule | If this style sheet is imported into the document using an @import rule, the ownerRule property will return that CSSImportRule, otherwise it returns null. |
| | attribute | cssRules | Returns a live CSSRuleList, listing the CSSRule objects in the style sheet. |
| | method | unsigned long insertRule(CSSOMString rule, optional unsigned long index = 0) | Inserts a new rule at the specified position in the style sheet, given the textual representation of the rule. |
| | method | void deleteRule(unsigned long index) | Deletes a rule at the specified position from the style sheet. |
| [CSSRuleList](https://drafts.csswg.org/cssom/#the-cssrulelist-interface) | interface | CSSRuleList | Represents an ordered collection of CSS style rules. |
| | method | getter CSSRule? item(unsigned long index) | Returns the indexth CSSRule object in the collection. |
| | attribute | length | Returns the number of CSSRule objects represented by the collection. |
| [Document](https://www.w3.org/TR/dom/#interface-document) | interface | Document | Also refer to Document [1](https://drafts.csswg.org/cssom/#extensions-to-the-document-interface), [2](https://www.w3.org/TR/dom/#interface-nonelementparentnode) and [3](https://www.w3.org/TR/dom/#parentnode)   |
|  | attribute | documentURI | Returns document's URL. |
|  | attribute | compatMode | Returns the string "CSS1Compat". |
|  | attribute | charset | Returns document's encoding type ""UTF8"". |
|  | attribute | characterSet | Returns document's encoding type ""UTF8"". |
|  | attribute | contentType | Returns document's content type "text/html". |
|  | attribute | doctype | Returns the doctype or null if there is none. |
|  | attribute | documentElement | Returns the document element. |
|  | method | HTMLCollection getElementsByTagName(DOMString qualifiedName) | If localName is "\*" returns an HTMLCollection of all descendant elements.Otherwise, returns an HTMLCollection of all descendant elements whose local name is localName. |
|  | method | HTMLCollection getElementsByClassName(DOMString classNames) | Returns an HTMLCollection of the elements in the object on which the method was invoked (a document or an element) that have all the classes given by classes. |
|  | method | Element createElement(DOMString localName) | Returns an element in the HTML namespace with localName as local name. |
|  | method | DocumentFragment createDocumentFragment() | Returns a new DocumentFragment node with its node document set to the context object. |
|  | method | Text createTextNode(DOMString data) | Returns a Text node whose data is data. |
|  | method | Comment createComment(DOMString data) | Returns a Comment node whose data is data. |
| [Document](https://dom.spec.whatwg.org/#interface-document) | method | CDATASection createCDATASection(DOMString data) | Returns a CDATASection node whose data is data. |
|  | method | Attr createAttribute(DOMString localName) | Return a new attribute whose local name is localName and node document is context object. |
| [Document](https://html.spec.whatwg.org/multipage/dom.html#the-document-object) | attribute | location | Return this Document object's relevant global object's Location object |
|  | attribute | body | Returns body element or null if not exists |
|  | attribute | head | Returns head element or null if not exists |
|  | attribute | defaultView | Returns this Document's browsing context's WindowProxy object, if this Document has an associated browsing context, or null otherwise |
|  | attribute | cookie | Represents the cookies of the resource identified by the document's URL. |
| [Document](https://drafts.csswg.org/cssom-view/#extensions-to-the-document-interface) | method | Element? elementFromPoint(double x, double y); | If there is a layout box in the viewport that would be a target for hit testing at coordinates x,y, return the associated element. If the document has a root element, returns the root element. Otherwise returns null |
| [Document](https://drafts.csswg.org/cssom/#extensions-to-the-document-interface) | attribute | styleSheets | Returns a StyleSheetList collection representing the document CSS style sheets. |
| [Document](https://www.w3.org/TR/page-visibility/#sec-document-interface) | attribute | hidden | Returns true if the Document contained by the top level browsing context (root window in the browser's viewport) is not visible at all. |
| | attribute | visibilityState | Returns one of the following strings: "hidden", or "visible" |
| [VisibilityChange Event](https://www.w3.org/TR/page-visibility/#sec-visibilitychange-event) | Event Handler | visibilitychange | Fire when the content of a tab has become visible or has been hidden. |
| [DocumentFragment](https://dom.spec.whatwg.org/#interface-documentfragment) | interface | DocumentFragment | DocumentFragment is a "lightweight" or "minimal" Document object. It is very common to want to be able to extract a portion of a document's tree or to create a new fragment of a document. |
| [DocumentType](https://dom.spec.whatwg.org/#documenttype) | interface | DocumentType | Document type |
|  | attribute | name | Return the context object’s name. |
|  | attribute | publicId | Return the context object’s public ID. |
|  | attribute | systemId | Return the context object’s system ID. |
| [DOMException](https://heycam.github.io/webidl/#idl-exceptions) | interface | DOMException |  |
|  | attribute | code | Exception code |
|  | attribute | name | optional exception name |
|  | attribute | message | optional exception message |
|  | constant | INDEX_SIZE_ERR = 1 | Deprecated. Use RangeError instead. |
|  | constant | HIERARCHY_REQUEST_ERR = 3 | The operation would yield an incorrect node tree. |
|  | constant | WRONG_DOCUMENT_ERR = 4 | The object is in the wrong document. |
|  | constant | INVALID_CHARACTER_ERR = 5 | The string contains invalid characters. |
|  | constant | NO_MODIFICATION_ALLOWED_ERR = 7 | The object can not be modified. |
|  | constant | NOT_FOUND_ERR = 8 | The object can not be found here. |
|  | constant | NOT_SUPPORTED_ERR = 9 | The operation is not supported. |
|  | constant | INUSE_ATTRIBUTE_ERR = 10 | The attribute is in use. |
|  | constant | INVALID_STATE_ERR = 11 | The object is in an invalid state. |
|  | constant | SYNTAX_ERR = 12 | The string did not match the expected pattern. |
|  | constant | INVALID_MODIFICATION_ERR = 13 | The object can not be modified in this way. |
|  | constant | NAMESPACE_ERR = 14 | The operation is not allowed by Namespaces in XML.  |
|  | constant | INVALID_ACCESS_ERR = 15 | Deprecated. Use TypeError for invalid arguments, "NotSupportedError" DOMException for unsupported operations, and "NotAllowedError" DOMException for denied requests instead. |
|  | constant | SECURITY_ERR = 18 | The operation is insecure. |
|  | constant | NETWORK_ERR = 19 | A network error occurred. |
|  | constant | ABORT_ERR = 20 | The operation was aborted. |
|  | constant | URL_MISMATCH_ERR = 21 | The quota has been exceeded.The given URL does not match another URL. |
|  | constant | QUOTA_EXCEEDED_ERR = 22 | The quota has been exceeded. |
|  | constant | TIMEOUT_ERR = 23 | The operation timed out. |
|  | constant | INVALID_NODE_TYPE_ERR = 24 | The supplied node is incorrect or has an incorrect ancestor for this operation. |
|  | constant | DATA_CLONE_ERR = 25 | The object can not be cloned. |
| [DOMParser](https://w3c.github.io/DOM-Parsing/#the-domparser-interface) | interface | DOMParser | DOMParser can parse XML or HTML source stored in a string into a DOM Document.  |
| | constructor | DOMParser() | Create a new DOMParser |
|  | enum | SupportedType | "text/html", "text/xml", "application/xml", "application/xhtml+xml", "image/svg+xml" |
| [DOMPoint](https://drafts.fxtf.org/geometry/#DOMPoint) | interface | DOMPoint |  |
|  | constructor | DOMPoint(optional unrestricted double x = 0, optional unrestricted double y = 0, optional unrestricted double z = 0, optional unrestricted double w = 1) | Creates a new DOMPoint object. |
|  | attribute | x | Return the x coordinate value of the object it was invoked on. |
|  | attribute | y | Return the y coordinate value of the object it was invoked on. |
|  | attribute | z | Return the z coordinate value of the object it was invoked on. |
|  | attribute | w | Return the w perspective value of the object it was invoked on. |
|  | dictionary | DOMPointInit::x | Initializes an DOMPoint object with x. |
|  | dictionary | DOMPointInit::y | Initializes an DOMPoint object with y. |
|  | dictionary | DOMPointInit::z | Initializes an DOMPoint object with z. |
|  | dictionary | DOMPointInit::w | Initializes an DOMPoint object with w. |
|  [DOMPointReadOnly](https://drafts.fxtf.org/geometry/#dompointreadonly)  |  attribute  |  x  |  Return  x coordinate value of the object  |
|    |  attribute  |  y  |  Return y coordinate value of the object  |
|    |  attribute  |  z  |  Return z coordinate value of the object  |
|    |  attribute  |  w  |  Return w perspective value of the object  |
| [DOMQuad](https://drafts.fxtf.org/geometry/#DOMQuad) | interface | DOMQuad | Objects implementing the DOMQuad interface represents a quadrilateral. |
| | constructor | DOMQuad(optional DOMPointInit p1, optional DOMPointInit p2, optional DOMPointInit p3, optional DOMPointInit p4) | |
|  | attribute | p1 | Return a DOMPoint that represents p1 of the quadrilateral |
|  | attribute | p2 | Return a DOMPoint that represents p2 of the quadrilateral |
|  | attribute | p3 | Return a DOMPoint that represents p3 of the quadrilateral |
|  | attribute | p4 | Return a DOMPoint that represents p4 of the quadrilateral |
|  | method | DOMRect getBounds() | Return bounds |
|  [DOMRect](https://drafts.fxtf.org/geometry/#domrect)  |  attribute  |  x  |  Return x coordinate value of the object   |
|    |  attribute  |  y  |  Return y coordinate value of the object   |
|    |  attribute  |  width  |  Return width dimension value of the object  |
|    |  attribute  |  height  |  Return height dimension value of the object  |
| [DOMRectList](https://dxr.mozilla.org/mozilla-central/source/dom/webidl/DOMRectList.webidl) | interface | DOMRectList | The DOMRectList objects are collections of DOMRects. DOMRectList must be supported for legacy reasons. New interfaces must not use DOMRectList and may use Sequences instead. |
|  | attribute | length | Returns the total number of DOMRect objects associated with the object. |
|  | method | DOMRect? item(unsigned long index) | Returns the DOMRect with the index number. |
|  [DOMRectReadOnly](https://drafts.fxtf.org/geometry/#domrectreadonly)  |  attribute  |  x  |  Return x coordinate value of the object   |
|    |  attribute  |  y  |  Return y coordinate value of the object   |
|    |  attribute  |  width  |  Return width dimension value of the object  |
|    |  attribute  |  height  |  Return height dimension value of the object  |
|    |  attribute  |  top  |  Return min(y coordinate, y coordinate + height dimension) of the object  |
|    |  attribute  |  right  |  Return max(x coordinate, x coordinate + width dimension) of the object  |
|    |  attribute  |  bottom  |  Return max(y coordinate, y coordinate + height dimension) of the object  |
|    |  attribute  |  left  |  Return min(x coordinate, x coordinate + width dimension) of the object  |
| [DOMSettableTokenList](https://dev.w3.org/html5/spec-LC/common-dom-interfaces.html#domsettabletokenlist-0) | interface | DOMSettableTokenList | The DOMSettableTokenList interface is the same as the DOMTokenList interface, except that it allows the underlying string to be directly changed. |
|  | attribute  | value | The value attribute must return the underlying string on getting, and must replace the underlying string with the new value on setting. |
|  [DOMTokenList](https://dom.spec.whatwg.org/#interface-domtokenlist)  |  attribute  |  length  |  Returns the number of tokens.  |
|    |  method  |  DOMString? item(unsigned long index) (or tokenlist[index])  |  Returns the token with the index index number.  |
|    |  method  |  boolean contains(DOMString token)  |  Returns true if token is present, and false otherwise.  |
|    |  method  |  void add(DOMString... tokens)  |  Adds all arguments passed, except those already present.  |
|    |  method  |  void remove(DOMString... tokens)  |  Removes arguments passed, if they are present.  |
|    |  method  |  boolean toggle(DOMString token, optional boolean force = false)  |  If force is not specified, "toggles" token, removing it if it is present and adding it if it is not. If force is true, adds token (same as add()). If force is false, removes token (same as remove()).   |
| [Element](https://dom.spec.whatwg.org/#interface-element) | interface | Element | Element nodes are simply known as elements. |
|  | attribute | namespaceURI | Return the context object’s namespace. |
|  | attribute | localName | Return the value of the attribute in element's attribute list whose namespace is namespace and local name is localName, if it has one, and null otherwise. |
|  | attribute | tagName | If namespace prefix is not null, returns the concatenation of namespace prefix, ":", and local name. Otherwise it returns the local name. |
|  | attribute | id | Reflects the "id" content attribute. |
|  | attribute | className | Reflects the "class" content attribute. |
|  | attribute | classList | Returns the associated DOMTokenList object representing the context object's classes. |
|  | attribute | attributes | Returns a NamedNodeMap. |
|  | method | DOMString? getAttribute(DOMString qualifiedName) | Returns the value of the first attribute in the context object's attribute list whose name is name, and null otherwise. |
|  | method | void setAttribute(DOMString qualifiedName, DOMString value) | Changes the attribute whose name is name from context object to value. |
|  | method | void removeAttribute(DOMString qualifiedName) | Removes the first attribute from the context object whose name is name, if any. |
|  | method | boolean hasAttribute(DOMString qualifiedName) | Returns true if the context object has an attribute whose name is name, and false otherwise. |
|  | method | HTMLCollection getElementsByTagName(DOMString qualifiedName) | Returns the list of elements with local name localName for the context object. |
|  | method | HTMLCollection getElementsByClassName(DOMString classNames) | Returns the list of elements with class names classNames for the context object. |
|| method | insertAdjacentElement |t inserts the node into the tree in the position given by the position argument |
|| method | insertAdjacentText | inserts the node into the tree in the position given by the position argument |
| [Element](https://w3c.github.io/DOM-Parsing/#extensions-to-the-element-interface) | attribute | innerHTML | Return a fragment of HTML or XML that represents the element's contents.|
|| attribute | outerHTML | Return a fragment of HTML or XML that represents the element|
|| method | insertAdjacentHTML | Parses the given string text as HTML or XML and inserts the resulting nodes into the tree in the position given by the position argument |
| [Element](https://drafts.csswg.org/cssom-view/#extension-to-the-element-interface) | method | getClientRects | Return a collection of rectangles that indicate the bounding rectangles for each box in a client. (Note: This API is supported only in case of that display property is `BLOCK`.)|
|  | method | getBoundingClientRect | Return the size of an element and its position relative to the viewport. (Note: This API is supported only in case of that display property is `BLOCK`.)|
|  | attribute | clientTop | Return the width of the top border of an element in pixels. |
|  | attribute | clientLeft | Return the width of the left border of an element in pixels. |
|  | attribute | clientWidth | Return zero for elements with no CSS or inline layout boxes, otherwise the inner width of an element in pixels. |
|  | attribute | clientHeight | Return zero for elements with no CSS or inline layout boxes, otherwise the inner height of an element in pixels. |
| [EventTarget](https://dom.spec.whatwg.org/#interface-eventtarget) | interface | EventTarget | Represents the target to which an event is dispatched when something has occurred. |
| | method | void addEventListener(DOMString type, EventListener? callback, optional boolean capture=false) | Adds the specified EventListener-compatible object to the list of event listeners for the specified event type on the EventTarget on which it's called. (NOTE: Starfish only support boolean type for third argument) |
| | method | void removeEventListener(DOMString type, EventListener? callback, optional boolean captures=false) | Removes from the EventTarget an event listener previously registered with EventTarget.addEventListener(). (NOTE: Starfish only support boolean type for third argument) |
| | method | boolean dispatchEvent(Event event) | Dispatches an Event at the specified EventTarget, invoking the affected EventListeners in the appropriate order. |
| [EventListener](https://dom.spec.whatwg.org/#callbackdef-eventlistener) | callback | EventListener = void () | An event listener can be used to observe a specific event. |
| [ElementCSSInlineStyle](https://drafts.csswg.org/cssom/#elementcssinlinestyle) | interface | ElementCSSInlineStyle | The ElementCSSInlineStyle interface provides access to inline style properties of an element. |
|  | attribute | style | Return a live CSS declaration block. |
| [HTMLAnchorElement](https://html.spec.whatwg.org/multipage/semantics.html#the-a-element) | interface | HTMLAnchorElement | The HTMLAnchorElement interface represents hyperlink elements and provides special properties and methods (beyond those of the regular HTMLElement object interface that they inherit from) for manipulating the layout and presentation of such elements. |
| [HTMLAudioElement](https://www.w3.org/TR/html5/embedded-content-0.html#the-audio-element) | interface | HTMLAudioElement | The audio element represents a sound or audio stream. (Note: Currently, elements related to multimedia are checked on Tizen 2.4 TV Product.)|
|  | constructor | Audio(optional DOMString src="") | |
| [HTMLBodyElement](https://html.spec.whatwg.org/multipage/semantics.html#the-body-element) | interface | HTMLBodyElement | The body element represents the main content of the document. |
|  | attribute | onload | Fired at the Window when the document has finished loading; fired at an element containing a resource (e.g. img, embed) when its resource has finished loading |
| [HTMLBRElement](https://html.spec.whatwg.org/multipage/semantics.html#the-br-element) | interface | HTMLBRElement | The br element represents a line break. |
|  [HTMLCollection](https://dom.spec.whatwg.org/#htmlcollection)  |  attribute  |  length  |  Returns the number of elements in the collection.  |
|    |  method  |  Element? item(unsigned long index) (or collection[index])  |  Returns the element with index index number from the collection. The elements are sorted in tree order.  |
| [HTMLDivElement](https://www.w3.org/TR/html5/grouping-content.html#the-div-element) | interface | HTMLDivElement | Offers a generic mechanism for adding structure to documents |
| [HTMLDocument](https://www.w3.org/TR/DOM-Level-2-HTML/html.html#ID-26809268) | interface | HTMLDocument | An HTMLDocument is the root of the HTML hierarchy and holds the entire content. |
| [HTMLElement 1](https://html.spec.whatwg.org/multipage/dom.html#htmlelement) | interface | HTMLElement |  |
|  | attribute | dir | Returns the dir attribute specifies the element's text directionality |
|  | method | void click() | Acts as if the element was clicked. |
|  | method | void focus() | When an element is focused, key events received by the document must be targeted at that element. |
| [HTMLElement 2](https://drafts.csswg.org/cssom-view/#extensions-to-the-htmlelement-interface) | attribute | offsetWidth | Returns the border edge width of the first CSS layout box associated with the element |
| | attribute | offsetHeight | Returns the border edge height of the first CSS layout box associated with the element |
| [HTMLHeadElement](https://html.spec.whatwg.org/multipage/semantics.html#the-head-element) | interface | HTMLHeadElement | The head element represents a collection of metadata for the Document. |
| [HTMLHtmlElement](https://html.spec.whatwg.org/multipage/semantics.html#the-html-element) | interface | HTMLHtmlElement | The html element represents the root of an HTML document. |
| [HTMLImageElement](https://html.spec.whatwg.org/multipage/embedded-content.html#the-img-element) | interface | HTMLImageElement | Represents an image. |
|  | constructor | Image(optional unsigned long width = 0, optional unsigned long height = 0) |  |
|  | attribute | src | Reflects the src HTML attribute, containing the full URL of the image including base URI. |
|  | attribute | width | Reflects the width HTML attribute, indicating the rendered width of the image in CSS pixels. |
|  | attribute | height | Reflects the height HTML attribute, indicating the rendered height of the image in CSS pixels. |
| [HTMLIFrameElement](https://html.spec.whatwg.org/multipage/iframe-embed-object.html#the-iframe-element) | interface | HTMLIFrameElement |  |
|  | attribute | src | Reflects the src HTML attribute, containing the full URL of the frame including base URI. |
|  | attribute | width | Reflects the width HTML attribute, indicating the rendered width of the frame in CSS pixels. |
|  | attribute | height | Reflects the height HTML attribute, indicating the rendered height of the frame in CSS pixels. |
| [HTMLLinkElement](https://html.spec.whatwg.org/multipage/semantics.html#the-link-element) | interface | HTMLLinkElement | The HTMLLinkElement interface represents reference information for external resources and the relationship of those resources to a document and vice-versa |
|  | attribute | href | Is a DOMString representing the URI for the target resource. |
|  | attribute | rel | Is a DOMString representing the forward relationship of the linked resource from the document to the resource. |
|  | attribute | media | Is a DOMString representing a list of one or more media formats to which the resource applies. |
|  | attribute | type | Is a DOMString representing the MIME type of the linked resource. |
| [HTMLUListElement](https://html.spec.whatwg.org/#htmlulistelement)  | interface | HTMLUListElement |  |
| [HTMLMediaElement](https://html.spec.whatwg.org/multipage/embedded-content.html#htmlmediaelement) | interface | HTMLMediaElement | The HTMLMediaElement interface adds to HTMLElement the properties and methods needed to support basic media-related capabilities that are common to audio and video. The HTMLVideoElement and HTMLAudioElement elements both inherit this interface. (Note: Currently, elements related to multimedia are checked on Tizen 2.4 TV Product.)|
|  | enum | CanPlayTypeResult | "", "maybe", "probably" |
|  | typedef | (MediaStream or MediaSource or Blob) MediaProvider |  |
|  | attribute | src | Is a DOMString that reflects the src HTML attribute, which contains the URL of a media resource to use. |
|  | attribute | currentSrc | Returns a DOMString with the absolute URL of the chosen media resource. |
|  | constant | NETWORK_EMPTY = 0 |  |
|  | constant | NETWORK_IDLE = 1 |  |
|  | constant | NETWORK_LOADING = 2 |  |
|  | constant | NETWORK_NO_SOURCE = 3 |  |
|  | attribute | networkState | Returns a unsigned short (enumeration) indicating the current state of fetching the media over the network. |
|  | attribute | preload | Is a DOMString that reflects the preload HTML attribute, indicating what data should be preloaded, if any. Possible values are: none, metadata, auto. |
|  | attribute | buffered | Returns a TimeRanges object that indicates the ranges of the media source that the browser has buffered (if any) at the moment the buffered property is accessed. |
|  | method |  void load() | Resets the media element and restarts the media resource. Any pending events are discarded. How much media data is fetched is still affected by the preload attribute. This method can be useful for releasing resources after any src attribute and source element descendants have been removed. Otherwise, it is usually unnecessary to use this method, unless required to rescan source element children after dynamic changes. |
|  | method | CanPlayTypeResult canPlayType(DOMString type) | Determines whether the specified media type can be played back. |
|  | constant | HAVE_NOTHING = 0 |  |
|  | constant | HAVE_METADATA = 1 |  |
|  | constant | HAVE_CURRENT_DATA = 2 |  |
|  | constant | HAVE_FUTURE_DATA = 3 |  |
|  | constant | HAVE_ENOUGH_DATA = 4 |  |
|  | attribute | readyState | Returns a unsigned short (enumeration) indicating the readiness state of the media. |
|  | attribute | seeking | Returns a TimeRanges object that contains the time ranges that the user is able to seek to, if any. |
|  | attribute | currentTime | Is a double indicating the current playback time in seconds. Setting this value seeks the media to the new time. |
|  | attribute | duration | Returns a double indicating the length of the media in seconds, or 0 if no media data is available. |
|  | attribute | paused | Returns a Boolean that indicates whether the media element is paused. |
|  | attribute | defaultPlaybackRate | Is a double indicating the default playback rate for the media. |
|  | attribute | playbackRate | Is a double that indicates the rate at which the media is being played back.  |
|  | attribute | played | Returns a TimeRanges object that contains the ranges of the media source that the browser has played, if any. |
|  | attribute | seekable | Returns a TimeRanges object that contains the time ranges that the user is able to seek to, if any. |
|  | attribute | ended | Returns a Boolean that indicates whether the media element has finished playing. |
|  | attribute | autoplay | A Boolean that reflects the autoplay HTML attribute, indicating whether playback should automatically begin as soon as enough media is available to do so without interruption. |
|  | attribute | loop | Is a Boolean that reflects the loop HTML attribute, which indicates whether the media element should start over when it reaches the end. |
|  | method | Promise\<void\> play() | Begins playback of the media. |
|  | method | void pause() | Pauses the media playback. |
|  | attribute | volume | Is a double indicating the audio volume, from 0.0 (silent) to 1.0 (loudest). |
|  | attribute | muted | Is a Boolean that determines whether audio is muted. true if the audio is muted and false otherwise. |
|  | attribute | textTracks | Returns the list of TextTrack objects contained in the element. |
|  | method | TextTrack addTextTrack(TextTrackKind kind, optional DOMString label = "", optional DOMString language = "") |  |
| [HTMLMetaElement](https://html.spec.whatwg.org/multipage/semantics.html#meta) | interface | HTMLMetaElement | The meta element represents various kinds of metadata that cannot be expressed using the title, base, link, style, and script elements. |
| [HTMLObjectElement](https://html.spec.whatwg.org/multipage/embedded-content.html#the-object-element) | interface | HTMLObjectElement | The object element can represent an external resource, which, depending on the type of the resource, will either be treated as an image, as a nested browsing context, or as an external resource to be processed by a plugin. |
| [HTMLParagraphElement](https://html.spec.whatwg.org/multipage/semantics.html#the-p-element)  | interface | HTMLParagraphElement |  |
| [HTMLPreElement](https://html.spec.whatwg.org/multipage/semantics.html#the-pre-element) | interface | HTMLPreElement | The HTMLPreElement interface expose specific properties and methods for manipulating block of preformatted text. |
| [HTMLScriptElement](https://html.spec.whatwg.org/multipage/scripting.html#the-script-element) | interface | HTMLScriptElement | The script element allows authors to include dynamic script and data blocks in their documents. |
|  | attribute | src | Address of the resource.<br>&lt;URL&gt; must be a local path. |
|  | attribute | type | Type of embedded resource.<br>Allowed value: text/javascript |
|  | attribute | charset | Character encoding of the external script resource.<br>Allowed value: UTF-8 |
|  | attribute | text | Return the child text content of the script element |
| [HTMLSourceElement](https://developer.mozilla.org/en-US/docs/Web/API/HTMLSourceElement) | interface | HTMLSourceElement | The HTMLSourceElement interface provides special properties for manipulating <source> elements. |
|  | attribute | src | DOMString reflecting the src HTML attribute, containing the URL for the media resource. (Note: Current version of HTMLSourceElement considers only media element related case, not picture case.) |
|  | attribute | type | DOMString reflecting the type HTML attribute, containing the type of the media resource. |
| [HTMLSpanElement](https://html.spec.whatwg.org/multipage/semantics.html#the-span-element) | interface | HTMLSpanElement | The span element is a generic inline container for phrasing content. |
| [HTMLStyleElement](https://html.spec.whatwg.org/multipage/semantics.html#the-style-element) | interface | HTMLStyleElement | The style element allows authors to embed style information in their documents. |
|  | attribute | media | Applicable media. |
|  | attribute | type | Type of embedded resource.<br>&lt;URL&gt; must be a local path.<br>Allowed value: text/css |
|  [HTMLTableCellElement](https://html.spec.whatwg.org/#htmltablecellelement)  |  attribute  |  colSpan  |  colspan content attribute  |
|    | attribute |  rowSpan  |  rowspan content attribute  |
|    | attribute |  bgColor  |  bgcolor content attributes  |
| [HTMLTableColElement](https://html.spec.whatwg.org/#htmltablecolelement) | interface | HTMLTableColElement |  |
|  | attribute | span | Number of columns spanned by the element. |
| [HTMLTableSectionElement](https://html.spec.whatwg.org/#htmltablesectionelement) | interface | HTMLTableSectionElement |  |
| [HTMLTrackElement](https://html.spec.whatwg.org/multipage/embedded-content.html#the-track-element) | interface | HTMLTrackElement | The track element allows authors to specify explicit external timed text tracks for media elements. It does not represent anything on its own. |
|  | attribute | kind | Return value of keywords such as subtitles, captions, descriptions, chapters and metadata. |
|  | attribute | src | Gives the URL of the text track data. |
|  | attribute | srclang | Gives the language of the text track data. |
|  | attribute | label | Gives a user-readable title for the track. |
|  | attribute | default | Indicates that the track is to be enabled if the user's preferences do not indicate that another track would be more appropriate. |
|  | constant | NONE | Indicates that the text track's cues have not been obtained. |
|  | constant | LOADING | Indicates that the text track is loading and there have been no fatal errors encountered so far. Further cues might still be added to the track by the parser. |
|  | constant | LOADED | Indicates that the text track has been loaded with no fatal errors. |
|  | constant | ERROR | Indicates that the text track was enabled, but when the user agent attempted to obtain it, this failed in some way. Some or all of the cues are likely missing and will not be obtained. |
|  | attribute | readyState | Returns the numeric value corresponding to the text track readiness state  |
|  | attribute | track | Returns the TextTrack object corresponding to the text track of the track element. |
| [HTMLVideoElement](https://html.spec.whatwg.org/#htmlvideoelement) | interface | HTMLVideoElement | A video element is used for playing videos or movies, and audio files with captions. (Note: Currently, elements related to multimedia are checked on Tizen 2.4 TV Product.)|
|  | attribute | width | Returns the dimensions of the visual content of the video. |
|  | attribute | height | Returns the dimensions of the visual content of the video. |
|  | attribute | videoWidth | Returns the intrinsic dimensions of the video, or zero if the dimensions are not known. |
|  | attribute | videoHeight | Returns the intrinsic dimensions of the video, or zero if the dimensions are not known. |
| [MediaList](https://drafts.csswg.org/cssom/#the-medialist-interface) | interface | MediaList | MediaList interface has an associated collection of media queries. |
|  | attribute | mediaText | Returns a serialization of the collection of media queries. |
|  | attribute | length | Returns the number of media queries in the collection of media queries. |
|  | method | getter CSSOMString? item(unsigned long index) | Returns a serialization of the media query in the collection of media queries given by index, or null, if index is greater than or equal to the number of media queries in the collection of media queries. |
|  | method | void appendMedium(CSSOMString medium) | Adds a media type to the mediaList collection. |
|  | method | void deleteMedium(CSSOMString medium) | Removes a media type from the mediaList collection. |
| [MediaQueryList](https://drafts.csswg.org/cssom-view/#mediaquerylist) | interface | MediaQueryList | A MediaQueryList object stores information on a media query applied to a document, and handles sending notifications to listeners when the media query state change (i.e. when the media query test starts or stops evaluating to true). |
|  | attribute | media | Return the associated media. |
|  | attribute | matches | Return the associated matches state. |
| [NamedNodeMap](https://dom.spec.whatwg.org/#interface-namednodemap) | interface | NamedNodeMap |  |
|  | attribute | length | Return the attribute list’s size. |
|  | method | Attr? item(unsigned long index) | Return the attribute at the given index, or null if the index is higher or equal to the number of nodes. |
|  | method | Attr? getNamedItem(DOMString qualifiedName) | Return the result of getting an attribute given qualifiedName and element. |
|  | method | Attr? setNamedItem(Attr attr) | Return the result of setting an attribute given attr and element. |
|  | method | Attr removeNamedItem(DOMString qualifiedName) | Remove the attribute identified by the given map. |
| [Node](https://dom.spec.whatwg.org/#interface-node) | interface | Node | Node is an abstract interface and does not exist as node. It is used by all nodes (Document, DocumentType, DocumentFragment, Element, Text, ProcessingInstruction, and Comment). |
|  | constant | ELEMENT_NODE | Node is an element. |
|  | constant | ATTRIBUTE_NODE | Node is an attribute |
|  | constant | TEXT_NODE | Node is a Text node. |
|  | constant | CDATA_SECTION_NODE | Node is a CDATASection node. |
|  | constant | ENTITY_REFERENCE_NODE | Node is an entry preference node |
|  | constant | ENTITY_NODE | Node is an entry node |
|  | constant | PROCESSING_INSTRUCTION_NODE | Node is a ProcessingInstruction node. |
|  | constant | COMMENT_NODE | Node is a Comment node. |
|  | constant | DOCUMENT_NODE | Node is a document. |
|  | constant | DOCUMENT_TYPE_NODE | Node is a doctype. |
|  | constant | DOCUMENT_FRAGMENT_NODE | Node is a DocumentFragment node. |
|  | constant | NOTATION_NODE | Node is a notation node |
|  | attribute | nodeType | Returns the node type |
|  | attribute | nodeName | Retuns the node name |
|  | attribute | ownerDocument | Returns the node document. Returns null for documents. |
|  | attribute | parentNode | Returns the parent. |
|  | attribute | parentElement | Returns the parent element. |
|  | method | boolean hasChildNodes() | Returns whether node has children. |
|  | attribute | childNodes | Returns the children. |
|  | attribute | firstChild | Returns the first child. |
|  | attribute | Node cloneNode(optional boolean deep = false) | Returns a copy of node. If deep is true, the copy also includes the node’s descendants. |
|  | attribute | previousSibling | Returns the previous sibling. |
|  | attribute | nextSibling | Returns the next sibling. |
|  | attribute | nodeValue | Gets and sets Attr, Text, ProcessingInstruction, Comment depending on the context object: |
|  | attribute | textContent | Gets and sets DocumentFragment, Element, Attr, Text, ProcessingInstruction, Comment switching on context object |
|  | method | normalize | Removes empty exclusive Text nodes and concatenates the data of remaining contiguous exclusive Text nodes into the first of their nodes. |
|  | method | boolean isEqualNode(Node? otherNode) | Returns whether node and otherNode have the same properties. |
|  | constant | DOCUMENT_POSITION_DISCONNECTED = 0x01; | Set when node and other are not in the same tree. |
|  | constant | DOCUMENT_POSITION_PRECEDING = 0x02; | Set when other is preceding node. |
|  | constant | DOCUMENT_POSITION_FOLLOWING = 0x04; | Set when other is following node. |
|  | constant | DOCUMENT_POSITION_CONTAINS = 0x08; | Set when other is an ancestor of node. |
|  | constant | DOCUMENT_POSITION_CONTAINED_BY = 0x10; | Set when other is a descendant of node. |
|  | constant | DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC = 0x20; |  |
|  | method | unsigned short compareDocumentPosition(Node other) | Returns a bitmask indicating the position of other relative to node.  |
|  | method | boolean contains(Node? other) | Returns true if other is an inclusive descendant of context object, and false otherwise |
|  | method | Node insertBefore(Node node, Node? child) | Returns the result of pre-inserting node into context object before child. |
|  | method | Node appendChild(Node node) | Returns the result of appending node to context object. |
|  | method | Node replaceChild(Node node, Node child) | Returns the result of replacing child with node within context object. |
|  | method | Node removeChild(Node child) | Returns the result of pre-removing child from context object. |
|  | attribute | lastChild | Returns the last child |
| [NodeList](https://dom.spec.whatwg.org/#nodelist) | interface | NodeList | A NodeList object is a collection of nodes. |
|  | method | Node? item(unsigned long index) | Returns the node with index index from the collection. The nodes are sorted in tree order. |
|  | attribute | length | Returns the number of nodes in the collection. |
|  | iterable&lt;Node&gt; |  |  |
| [NonDocumentTypeChildNode](https://dom.spec.whatwg.org/#nondocumenttypechildnode) | interface | NonDocumentTypeChildNode | The NonDocumentTypeChildNode interface contains methods that are particular to Node Object that can have a sibling. |
|  | attribute | previousElementSibling | Returns the Element immediately prior to this node in its parent's children list, or null if there is no Element in the list prior to this node. |
|  | attribute | nextElementSibling | Returns the Element immediately following this node in its parent's children list, or null if there is no Element in the list following this node. |
| [NonElementParentNode](https://www.w3.org/TR/dom/#interface-nonelementparentnode) | interface | NonElementParentNode |  |
|  | method | Element? getElementById(DOMString elementId) | Returns the first element within node's descendants whose ID is elementId. |
| [ParentNode](none) | interface | ParentNode | The ParentNode interface contains methods that are particular to Node objects that can have children. |
|  | attribute | firstElementChild | Returns the Element that is the first child of this ParentNode, or null if there is none. |
|  | attribute | lastElementChild | Returns the Element that is the last child of this ParentNode, or null if there is none. |
|  | attribute | childElementCount | Returns an unsigned long giving the amount of children that the object has. |
|  | method | Element? querySelector(DOMString selectors) | Returns the first Element with the current element as root that matches the specified group of selectors. |
|  | method | NodeList querySelectorAll(DOMString selectors) | Returns a NodeList representing a list of elements with the current element as root that matches the specified group of selectors. |
| [Text](https://dom.spec.whatwg.org/#text) | interface | Text | Text node whose data is data and node document is current global object’s associated Document. |
|  | attribute | wholeText | Returns the combined data of all direct Text node siblings. |
| [TextTrack](https://html.spec.whatwg.org/#texttrack)  | interface | TextTrack |  |
|  | enum | TextTrackMode | "disabled",  "hidden",  "showing" |
|  | enum | TextTrackKind | "subtitles",  "captions",  "descriptions",  "chapters",  "metadata" |
|  | attribute  | kind | Returns the text track kind string. |
|  | attribute  | label | Returns the text track label, if there is one, or the empty string otherwise  |
|  | attribute  | language | Returns the text track language string. |
|  | attribute  | id | Returns the ID of the given track. |
|  | attribute  | mode | Gets and sets the text track mode |
|  | attribute  | cues | Returns the text track list of cues, as a TextTrackCueList object. |
|  | attribute  | activeCues | Returns a live TextTrackCueList object  |
|  | method | void addCue(TextTrackCue cue) | Adds cue to the method's TextTrack object's text track's text track list of cues. |
|  | method | void removeCue(TextTrackCue cue) | Removes cue from the method's TextTrack object's text track's text track list of cues. |
|  | attribute  | oncuechange | The event handler for the cue change event  |
| [TextTrackCue](https://html.spec.whatwg.org/#texttrackcue) | interface | TextTrackCue | A text track cue is the unit of time-sensitive data in a text track, corresponding for instance for subtitles and captions to the text that appears at a particular time and disappears at another time. |
|  | attribute | track | Returns the TextTrack object to which this text track cue belongs, if any, or null otherwise. |
|  | attribute | id | Gets and sets the text track cue identifier. |
|  | attribute | startTime | Gets and setsthe text track cue start time, in seconds. |
|  | attribute | endTime | Gets and sets the text track cue end time, in seconds. |
|  | attribute | onenter | The event handler for the enter event  |
|  | attribute | onexit | The event handler for the exit event  |
|  [TextTrackCueList](https://html.spec.whatwg.org/#texttrackcuelist)  |  attribute  |  length |  Return the number of cues in the list represented by the TextTrackCueList object  |
|    |  method  |  TextTrackCue[unsigned long index]  |  Return Text track cue object with index  |
| [TextTrackList](https://html.spec.whatwg.org/#texttracklist) | interface | TextTrackList | A TextTrackList object represents a dynamically updating list of text tracks in a given order. |
|  | attribute | length | Returns the number of text tracks associated with the media element. |
|  | method | TextTrack (unsigned long index) | Returns the TextTrack object representing the nth text track in the media element's list of text tracks. |
|  | method | TextTrack? getTrackById(DOMString id) | Returns the TextTrack object with the given identifier, or null if no track has that identifier. |
| [VTTCue](https://w3c.github.io/webvtt/#vttcue) | interface | VTTCue | VTTCues represent a cue in a text track. |
| | constructor | VTTCue(double startTime, double endTime, DOMString text) | Create a new VTTCue |
|  | enum | AutoKeyword | "auto" |
|  | typedef | (double or AutoKeyword) LineAndPositionSetting |  |
|  | enum | DirectionSetting | "", "rl", "lr" |
|  | enum | LineAlignSetting | "start", "center", "end" |
|  | enum | PositionAlignSetting | "line-left", "center", "line-right", "auto" |
|  | enum | AlignSetting | "start", "center", "end", "left", "right" |
|  | attribute | text | Return the raw text track cue text of the WebVTT cue that the VTTCue object represents. On setting, the text track cue text must be set to the new value. |
|  | method | DocumentFragment getCueAsHTML() | Convert the text track cue text to a DocumentFragment for the responsible document specified by the entry settings object by applying the WebVTT cue text DOM construction rules to the result of applying the WebVTT cue text parsing rules to the text track cue text. |
| [XMLDocument](https://www.w3.org/TR/dom/#interface-document) | interface | XMLDocument | The XMLDocument interface represent an XML document. |
| [History](https://html.spec.whatwg.org/multipage/browsers.html#the-history-interface) | interface | History | The History interface allows to manipulate the browser session history, that is the pages visited in the tab or frame that the current page is loaded in. |
|  | attribute | length | Returns an Integer representing the number of elements in the session history, including the currently loaded page. For example, for a page loaded in a new tab this property returns 1. |
|  | attribute | state | Returns an any value representing the state at the top of the history stack. This is a way to look at the state without having to wait for a popstate event. |
|  | method | void go(optional long delta = 0) | Loads a page from the session history, identified by its relative location to the current page, for example -1 for the previous page or 1  for the next page. |
|  | method | void back() | Goes to the previous page in session history, the same action as when the user clicks the browser's Back button. Equivalent to history.go(-1). |
|  | method | void forward() | Goes to the next page in session history, the same action as when the user clicks the browser's Forward button; this is equivalent to history.go(1). |
|  | method | void pushState(any data, DOMString title, optional DOMString? url = null) | Pushes the given data onto the session history stack with the specified title and, if provided, URL. |
|  | method | void replaceState(any data, DOMString title, optional DOMString? url = null) | Updates the most recent entry on the history stack to have the specified data, title, and, if provided, URL |
|  [Location](https://html.spec.whatwg.org/multipage/browsers.html#location)  |  attribute  |  href  |  Return Location object's url  |
|    |  attribute  |  protocol  |  Return  Location object's url's scheme, followed by ":"  |
|    |  attribute  |  host  |  Return url's host, serialized, followed by ":" and url's port, serialized  |
|    |  attribute  |  pathname  |  Return "/", followed by the strings in url's path (including empty strings), separated from each other by "/"  |
|    |  attribute  |  search  |  Return "?", followed by this Location object's url's query  |
|    |  attribute  |  hash  |  Return "#", followed by this Location object's url's fragment  |
|    |  method  |  assign(DOMString url)  |  Loads the resource at the URL provided in parameter.  |
|    |  method  |  replace(DOMString url)  |  Replaces the current resource with the URL provided in parameter.  |
|    |  method  |  reload()  |  Reloads the resource from the current URL.  |
| [MediaSource](https://w3c.github.io/media-source/#mediasource) | enum | ReadyState | "closed", "open", "ended" |
|  | enum | EndOfStreamError  | "network", "decode" |
|  | interface | MediaSource | The MediaSource object represents a source of media data for an HTMLMediaElement. |
|  | attribute | sourceBuffers | Contains the list of SourceBuffer objects associated with this MediaSource. |
|  | attribute | activeSourceBuffers | Contains the subset of sourceBuffers that are providing the selected video track, the enabled audio track(s), and the "showing" or "hidden" text track(s). |
|  | attribute | readyState | Indicates the current state of the MediaSource object. |
|  | attribute | duration | Allows the web application to set the presentation duration. |
|  | method | SourceBuffer addSourceBuffer(DOMString type) | Adds a new SourceBuffer to sourceBuffers. |
|  | method | void removeSourceBuffer(SourceBuffer sourceBuffer) | Removes a SourceBuffer from sourceBuffers. |
|  | method | void endOfStream(optional EndOfStreamError error) | Signals the end of the stream. |
|  | method | static boolean isTypeSupported(DOMString type) | Check to see whether the MediaSource is capable of creating SourceBuffer objects for the specified MIME type. |
|  [SourceBuffer](https://w3c.github.io/media-source/#sourcebuffer)  |  attribute  |  mode  |  Controls how a sequence of media segments are handled  |
|    |  attribute  |  updating  |  Return whether the asynchronous continuation of an appendBuffer() or remove() operation is still being processed  |
|    |  attribute  |  buffered  |  Return what TimeRanges are buffered in the SourceBuffer  |
|    |  attribute  |  timestampOffset  |  Controls the offset applied to timestamps inside subsequent media segments that are appended to this SourceBuffer  |
|    |  attribute  |  textTracks  |  Return The list of TextTrack objects created by this object  |
|    |  attribute  |  appendWindowStart  |  The presentation timestamp for the start of the append window  |
|    |  attribute  |  appendWindowEnd  |  The presentation timestamp for the end of the append window  |
|    |  attribute  |  onupdatestart  |  The event handler for the updatestart event  |
|    |  attribute  |  onupdate  |  The event handler for the update event  |
|    |  attribute  |  onupdateend  |  The event handler for the updateend event  |
|    |  attribute  |  onerror  |  The event handler for the error event  |
|    |  attribute  |  onabort  |  The event handler for the abort event  |
|    |  method  |  void appendBuffer(BufferSource data)  |  Appends the segment data in an BufferSource to the source buffer |
|    |  method  |  void abort()  |  Aborts the current segment and resets the segment parser  |
|    |  method  |  void remove(double start, unrestricted double end)  |  Removes media for a specific time range  |
|  [SourceBufferList](https://w3c.github.io/media-source/#sourcebufferlist)  |  attribute  |  length |  Return number of SourceBuffer objects in the list.  |
|    |  method  |  SourceBuffer[unsigned long index]  |  Return SourceBuffer object with index  |
| [StyleSheet](https://drafts.csswg.org/cssom/#the-stylesheet-interface) | interface | StyleSheet | The StyleSheet interface represents an abstract, base style sheet. |
| | attribute | type | Specifies the style sheet language for this style sheet. |
| | attribute | href | If the style sheet is a linked style sheet, the value of its attribute is its location. |
| | attribute | parentStyleSheet | Returns the parent CSS style sheet. |
| | attribute | ownerNode | The node that associates this style sheet with the document. |
| [StyleSheetList](https://drafts.csswg.org/cssom/#the-stylesheetlist-interface) | interface | StyleSheetList | The StyleSheetList interface represents an ordered collection of CSS style sheets. |
| | method | getter StyleSheet? item(unsigned long index) | Return the indexth CSS style sheet in the collection. |
| | attribute | length | Return the number of CSS style sheets represented by the collection. |
| [TimeRanges](https://html.spec.whatwg.org/multipage/embedded-content.html#time-ranges) | interface | TimeRanges | The TimeRanges interface represent a list of ranges (periods) of time. |
|  | attribute | length | Returns the number of ranges in the object. |
|  | method | double start(unsigned long index) | Returns the time for the start of the range with the given index. |
|  | method | double end(unsigned long index) | Returns the time for the end of the range with the given index. |
| [Touch](https://w3c.github.io/touch-events/#idl-def-touch) | interface | Touch | Describes an individual touch point for a touch event. |
| | attribute | target | The EventTarget on which the touch point started when it was first placed on the surface. |
| | attribute | screenX | The horizontal coordinate of point relative to the screen in pixels. |
| | attribute | screenY | The vertical coordinate of point relative to the screen in pixels. |
| | attribute | clientX | The horizontal coordinate of point relative to the viewport in pixels, excluding any scroll offset. |
| | attribute | clientY | The vertical coordinate of point relative to the viewport in pixels, excluding any scroll offset. |
| [TouchInit](https://w3c.github.io/touch-events/#idl-def-touchinit) | dictionary | TouchInit | Dictionary that is used to create TouchInit. |
| | attrbitue | target | Initializes the target attribute of the Touch object |
| | attrbitue | screenX | Initializes the screenX attribute of the Touch object |
| | attrbitue | screenY | Initializes the screenY attribute of the Touch object |
| | attrbitue | clientX | Initializes the clientX attribute of the Touch object |
| | attrbitue | clientY | Initializes the clientY attribute of the Touch object |
| [TouchList](https://w3c.github.io/touch-events/#idl-def-touchlist) | interface | TouchList | Defines a list of individual points of contact for a touch event. |
| | attribute | length | Returns the number of Touch objects in the list |
| [Window](https://html.spec.whatwg.org/#the-window-object) | interface | Window | The Window has an associated Document, which is a Document object. |
|  | attribute | window | Returns window. |
|  | attribute | self | Returns window. |
|  | attribute | document | Returns the document associated with window. |
|  | attribute | location | Return this Window object's Location object. |
|  | attribute | history | Return the object implementing the History interface for this Window object's associated Document. |
|  | attribute | navigator | Return an instance of the Navigator interface, which represents the identity and state of the user agent (the client), and allows Web pages to register themselves as potential protocol and content handlers |
| [Window](https://www.w3.org/TR/cssom-view-1/#extensions-to-the-window-interface) | attribute | innerWidth | Return the viewport width including the size of a rendered scroll bar (if any), or zero if there is no viewport.  |
|  | attribute | innerHeight | Return the viewport height including the size of a rendered scroll bar (if any), or zero if there is no viewport.  |
| [Window](https://www.w3.org/TR/animation-timing/#Window-interface-extensions) | method | unsigned long requestAnimationFrame(FrameRequestCallback callback) | Used to signal to the user agent that a script-based animation needs to be resampled. |
| | method | void cancelAnimationFrame(unsigned long handle) | Used to cancel a previously made request to schedule an animation frame update. |
| | callback | FrameRequestCallback = void (DOMHighResTimeStamp time) | |
| [Window](https://drafts.csswg.org/cssom/#extensions-to-the-window-interface) | method | CSSStyleDeclaration getComputedStyle(Element elt, optional CSSOMString? pseudoElt) | Gives the values of all the CSS properties of an element after applying the active stylesheets and resolving any basic computation those values may contain. |
| [Window](https://drafts.csswg.org/cssom-view/#extensions-to-the-window-interface) | method | MediaQueryList matchMedia(CSSOMString query) | Returns a new MediaQueryList object representing the parsed results of the specified media query string. |
| [Named Access on the Window Object](https://html.spec.whatwg.org/multipage/browsers.html#named-access-on-the-window-object) | misc | window[id] | Named access on the Window object returns the indicated element, where id is a non-empty ID of an HTML element in the current document. |
| [URL](https://url.spec.whatwg.org/#url) | interface | URL | The URLinterface represent an object providing static methods used for creating object URLs. |
| | constructor | URL(DOMString url, optional DOMString base) | Create a new URL |
|  | attribute | href | A DOMString containing the whole URL. |
|  | attribute | origin | A DOMString containing the origin of the URL, that is its scheme, its domain and its port. |
|  | attribute | protocol | A DOMString containing the protocol scheme of the URL, including the final ':'. |
|  | attribute | username | A DOMString containing the username specified before the domain name. |
|  | attribute | password | A DOMString containing the password specified before the domain name. |
|  | attribute | host | A DOMString containing the host, that is the hostname, a ':', and the port of the URL. |
|  | attribute | hostname | A DOMString containing the domain of the URL. |
|  | attribute | port | A DOMString containing the port number of the URL. |
|  | attribute | pathname | A DOMString containing an initial '/' followed by the path of the URL. |
|  | attribute | search | A DOMString containing a '?' followed by the parameters of the URL. |
|  | attribute | hash | A DOMString containing a '#' followed by the fragment identifier of the URL. |
| [WindowTimers](https://www.w3.org/TR/html5/webappapis.html#timers) | method | long setTimeout(TimerHandler handler, optional long timeout = 0, any... arguments) | Calls a function or evaluates an expression after a specified number of milliseconds. |
|  | method | static DOMString createObjectURL(Blob blob) | Returns a DOMString containing a unique blob URL, that is a URL with blob: as its scheme, followed by an opaque string uniquely identifying the object in the browser. |
|  | method | static DOMString createObjectURL(MediaSource mediaSource) | Returns a DOMString containing a unique blob URL, that is a URL with media source: as its scheme, followed by an opaque string uniquely identifying the object in the browser. |
|  | method | static void revokeObjectURL(DOMString url) | Revokes an object URL previously created using URL.createObjectURL() |
|  | method | void clearTimeout(optional long handle = 0) | Clears a timer set with setTimeout(). |
|  | method | long setInterval(TimerHandler handler, optional long timeout = 0, any... arguments) | Calls a function or evaluates an expression at specified intervals (in milliseconds). |
|  | method | void clearInterval(optional long handle = 0) | Clears a timer set with setInterval(). |
|  | typedef | (DOMString or Function) TimerHandler | |


## Event
| Interface | Type | Name | Description |
|-----------|------|------|-------------|
| [Event](https://dom.spec.whatwg.org/#interface-event) | interface | Event | |
| | constructor | Event(DOMString type, optional EventInit eventInitDict) | Creates a new Event object. |
| | constant | NONE = 0 | Events not currently dispatched are in this phase. |
| | constant | CAPTURING_PHASE = 1 | When an event is dispatched to an object that participates in a tree it will be in this phase before it reaches its target attribute value. |
| | constant | AT_TARGET = 2 | When an event is dispatched it will be in this phase on its target attribute value. |
| | constant | BUBBLING_PHASE = 3 | When an event is dispatched to an object that participates in a tree it will be in this phase after it reaches its target attribute value. |
| | attribute | bubbles | Returns true or false depending on how event was initialized. True if event goes through its target attribute value’s ancestors in reverse tree order, and false otherwise. |
| | attribute | cancelable | Returns true or false depending on how event was initialized. Its return value does not always carry meaning, but true can indicate that part of the operation during which event was dispatched, can be canceled by invoking the preventDefault() method. |
| | attribute | currentTarget | Returns the object whose event listener’s callback is currently being invoked. |
| | attribute | defaultPrevented | Returns true if preventDefault() was invoked successfully to indicate cancellation, and false otherwise. |
| | attribute | eventPhase | Returns the event’s phase, which is one of NONE, CAPTURING_PHASE, AT_TARGET, and BUBBLING_PHASE. |
| | attribute | target | Returns the object to which event is dispatched. |
| | attribute | timeStamp | Returns the creation time of event as the number of milliseconds that passed since 00:00:00 UTC on 1 January 1970. |
| | attribute | type | Returns the type of event, e.g. "click, "hashchange", or "submit" |
| | method | void stopPropagation() | When dispatched in a tree, invoking this method prevents event from reaching any objects other than the current object. |
| | method | void stopImmediatePropagation() | Invoking this method prevents event from reaching any registered event listeners after the current one finishes running and, when dispatched in a tree, also prevents event from reaching any other objects. |
| | method | void preventDefault() | If invoked when the cancelable attribute value is true, and while executing a listener for the event with passive set to false, signals to the operation that caused event to be dispatched that it needs to be canceled. |
| | dictionary | EventInit::bubles = false | Initializes an Event object with bubbles. |
| | dictionary | EventInit::cancelable = false | Initializes an Event object with cancelable. |
| [FocusEvent](https://w3c.github.io/uievents/#interface-focusevent) | interface | FocusEvent | The FocusEvent interface represents focus-related events like focus, blur, focusin, or focusout. |
| | constructor | FocusEvent(DOMString type, optional FocusEventInit eventInitDict) | Create a new FocusEvent |
| | attribute | relatedTarget | Used to identify a secondary EventTarget related to a Focus event, depending on the type of event. |
| [GlobalEventHandlers](https://html.spec.whatwg.org/multipage/webappapis.html#globaleventhandlers) | partial<br>interface | GlobalEventHandlers | The GlobalEventHandlers are the event handlers common to several interfaces like HTMLElement, Document, or Window. |
| | attribute | onabort | Fired at the Window when the download was aborted by the user |
| | attribute | oncanplay | Fired when the user agent can resume playback of the media data, but estimates that if playback were to be started now, the media resource could not be rendered at the current playback rate up to its end without having to stop for further buffering of content. |
| | attribute | oncanplaythrough | Fired when the user agent estimates that if playback were to be started now, the media resource could be rendered at the current playback rate all the way to its end without having to stop for further buffering. |
| | attribute | onclick | Fired when the click event is raised. |
| | attribute | ondurationchange | Fired when the duration attribute has just been updated. |
| | attribute | onemptied | Fired when a media element whose networkState was previously not in the NETWORK_EMPTY state has just switched to that state. |
| | attribute | onended | Fired when playback has stopped because the end of the media resource was reached. |
| | attribute | onerror | Fired when the error event is raised. |
| | attribute | onfocus | Fired when the focus event is raised. |
| | attribute | onkeydown | Fired when the keydown event is raised. |
| | attribute | onkeyup | Fired when the keyup event is raised. |
| | attribute | onload | Fired when the load event is raised. |
| | attribute | onloadeddata | Fired when the user agent can render the media data at the current playback position for the first time. |
| | attribute | onloadedmetadata | Fired when the user agent has just determined the duration and dimensions of the media resource and the text tracks are ready. |
| | attribute | onloadstart | Fired when the user agent begins looking for media data, as part of the resource selection algorithm. |
| | attribute | onmouseover | Fired when the mouseover event is raised. |
| | attribute | onpause | Fired when the element has been paused. |
| | attribute | onplay | Fired when the element is no longer paused. Fired after the play() method has returned, or when the autoplay attribute has caused playback to begin. |
| | attribute | onplaying | Fired when playback is ready to start after having been paused or delayed due to lack of media data. |
| | attribute | onprogress | Fired when the user agent is fetching media data. |
| | attribute | onratechange | Fired when either the defaultPlaybackRate or the playbackRate attribute has just been updated. |
| [MouseEvent](https://w3c.github.io/uievents/#idl-mouseevent) | interface | MouseEvent |  |
| | attribute | screenX | The horizontal coordinate at which the event occurred relative to the origin of the screen |
| | attribute | screenY | The vertical coordinate at which the event occurred relative to the origin of the screen |
| | attribute | clientX | The horizontal coordinate at which the event occurred relative to the viewport |
| | attribute | clientY | The vertical coordinate at which the event occurred relative to the viewport |
| [MouseEventInit](https://w3c.github.io/uievents/#idl-mouseeventinit) | dictionary | MouseEventInit |  |
| | attribute | screenX | Initializes the screenX attribute of the MouseEvent object |
| | attribute | screenY | Initializes the screenY attribute of the MouseEvent object |
| | attribute | clientX | Initializes the clientX attribute of the MouseEvent object |
| | attribute | clientY | Initializes the clientY attribute of the MouseEvent object |
| [KeyboardEvent](https://w3c.github.io/uievents/#interface-keyboardevent) | interface | KeyboardEvent | KeyboardEvent objects describe a user interaction with the keyboard. Each event describes a key; the event type (keydown, keypress, or keyup) identifies what kind of activity was performed. |
|  | constant | DOM_KEY_LOCATION_STANDARD = 0x00 |  |
|  | constant | DOM_KEY_LOCATION_LEFT = 0x01 |  |
|  | constant | DOM_KEY_LOCATION_RIGHT = 0x02 |  |
|  | constant | DOM_KEY_LOCATION_NUMPAD = 0x03 |  |
|  | attribute | ctrlKey | Returns a Boolean that is true if the Ctrl key was active when the key event was generated. |
|  | attribute | shiftKey | Returns a Boolean that is true if the Shift key was active when the key event was generated. |
|  | attribute | altKey | Returns a Boolean that is true if the Alt key was active when the key event was generated. |
|  | attribute | metaKey | Returns a Boolean that is true if the Meta key was active when the key event was generated. |
|  | attribute | keyCode | Returns a Number representing a system and implementation dependent numerical code identifying the unmodified value of the pressed key. |
| [ProgressEvent](https://www.w3.org/TR/progress-events/#interface-progressevent) | interface | ProgressEvent | The ProgressEvent interface represents events measuring progress of an underlying process, like an HTTP request (for an XMLHttpRequest, or the loading of the underlying resource of an \<img\>, \<audio\>, \<video\>, \<style\> or \<link\>). |
| | constructor | ProgressEvent(DOMString type, optional FocusEventInit eventInitDict) | Create a new ProgressEvent |
| | attribute | lengthComputable | Is a Boolean flag indicating if the total work to be done, and the amount of work already done, by the underlying process is calculable. In other words, it tells if the progress is measurable or not. |
| | attribute | loaded | Is an unsigned long long representing the amount of work already performed by the underlying process. The ratio of work done can be calculated with the property and ProgressEvent.total. When downloading a resource using HTTP, this only represent the part of the content itself, not headers and other overhead. |
| | attribute | total | Is an unsigned long long representing the total amount of work that the underlying process is in the progress of performing. When downloading a resource using HTTP, this only represent the content itself, not headers and other overhead. |
| [UIEvent](https://w3c.github.io/uievents/#interface-UIEvent) | interface | UIEvent | The UIEvent interface provides specific contextual information associated with User Interface events. |
| | attribute | view | The view attribute identifies the Window from which the event was generated |
| [UIEventInit](https://w3c.github.io/uievents/#dictdef-uieventinit) | dictionary | UIEventInit | Dictionary that is used to create UIEvent. |
| | attrbitue | view | Should be initialized to the Window object of the global environment in which this event will be dispatched |
| [EventModifierInit](https://w3c.github.io/uievents/#dictdef-eventmodifierinit) | dictionary | EventModifierInit | The MouseEvent and KeyboardEvent interfaces share a set of keyboard modifier attributes. EventModifierInit enables authors to initialize keyboard modifier attributes of the MouseEvent and KeyboardEvent interfaces. |
| | attrribute | ctrlKey | true if the Control key modifier is to be considered active, false otherwise |
| | attrribute | shiftKey | true if the Shift key modifier is to be considered active, false otherwise. |
| | attrribute | altKey | true if the Alt (alternative) (or Option) key modifier is to be considered active, false otherwise. |
| | attrribute | metaKey | true if the Meta key modifier is to be considered active, false otherwise. |
| [TouchEvent](https://w3c.github.io/touch-events/#touchevent-interface) | interface | TouchEvent | Defines the touchstart, touchend, touchmove, and touchcancel event types. |
| | attribute | touches | A list of Touch objects for every point of contact currently touching the surface. |
| [WindowEventHandlers](https://html.spec.whatwg.org/multipage/webappapis.html#windoweventhandlers) | partial<br>interface | WindowEventHandlers | WindowEventHandlers are the event handlers common to several interfaces like Window, or HTMLBodyElement and  HTMLFrameSetElement. Each of these interfaces can implement additional specific event handlers. |
| | attribute | onunload | Represents the code to be called when the unload event is raised. |

## CSS

| Type | Property | Allowed Value | Description | Note |
|------|----------|---------------|-------------|------|
| [Margin](https://www.w3.org/TR/CSS2/box.html#margin-properties) | margin | &lt;margin-width&gt;{1,4} | The margin shorthand property sets all the margin properties in one declaration | The margin properties specify the width of the margin area of a box. &lt;margin-width&gt; may take one of the following values: &lt;length&gt;, &lt;percentage&gt;, auto. (Also check [Length](https://www.w3.org/TR/CSS2/syndata.html#length-units)) |
| | margin-bottom | &lt;margin-width&gt; &#124; &lt;percentage&gt; &#124; auto | Sets the bottom margin of an element | |
| | margin-left | &lt;length&gt; &#124; &lt;percentage&gt; &#124; auto | Sets the left margin of an element | |
| | margin-right | &lt;length&gt; &#124; &lt;percentage&gt; &#124; auto | Sets the right margin of an element | |
| | margin-top | &lt;length&gt; &#124; &lt;percentage&gt; &#124; auto | Sets the top margin of an element | |
| [Padding](https://www.w3.org/TR/CSS2/box.html#padding-properties) | padding | &lt;padding-width&gt;{1,4} | The padding shorthand property sets all the padding properties in one declaration | &lt;padding-width&gt; may take one of the following values: &lt;length&gt;, &lt;percentage&gt; |
| | padding-bottom | &lt;length&gt; &#124; &lt;percentage&gt;	| Sets the bottom padding for an element | |
| | padding-left | &lt;length&gt; &#124; &lt;percentage&gt;	| Sets the left padding for an element | |
| | padding-right | &lt;length&gt; &#124; &lt;percentage&gt; | Sets the right padding for an element | |
| | padding-top | &lt;length&gt; &#124; &lt;percentage&gt; | Sets the top padding for an element | |
| [Border](https://www.w3.org/TR/css3-border/) | border | &lt;border-width&gt; &lt;border-style&gt; &lt;border-color&gt; | Sets all the border properties (shorthand). | The border can either be a predefined style (solid line) or it can be an image. In the former case, various properties define the style (&lt;border-style&gt;), color (&lt;border-color&gt;), and thickness (&lt;border-width&gt;) of the border. &lt;border-width&gt; may take one of the following values: thin, medium, thick, &lt;length&gt; &lt;border-color&gt; may take one of the following values: &lt;color&gt;, transparent &lt;border-style&gt; may take one of the following values: none, solid (Also check Border Properties) |
| | border-bottom | &lt;border-width&gt;   &lt;border-style&gt;   &lt;border-color&gt; | Sets all the bottom border properties (shorthand). | |
| | border-bottom-color | &lt;color&gt; &#124; transparent | Sets the color of the bottom border. | |
| | border-bottom-style | none &#124; solid | Sets the style of the bottom border. | |
| | border-bottom-width | medium &#124; thin &#124; thick &#124; &lt;length&gt; | Sets the width of the bottom border. | |
| | border-color | &lt;border-color&gt;{1,4} | Sets the color of the four borders (shorthand). | |
| | border-left | &lt;border-width&gt;   &lt;border-style&gt;   &lt;border-color&gt; | Sets all the left border properties (shorthand). | |
| | border-left-color | &lt;color&gt; &#124; transparent | Sets the color of the left border. | |
| | border-left-style | none &#124; solid | Sets the style of the left border. | |
| | border-left-width | medium &#124; thin &#124; thick &#124; &lt;length&gt; | Sets the width of the left border. | |
| | border-right | &lt;border-width&gt;   &lt;border-style&gt;   &lt;border-color&gt;	| Sets all the right border properties (shorthand). | |
| | border-right-color | &lt;color&gt; &#124; transparent | Sets the color of the right border. | |
| | border-right-style | none &#124; solid | Sets the style of the right border. | |
| | border-right-width | medium &#124; thin &#124; thick &#124; &lt;length&gt;	| Sets the width of the left border. | |
| | border-style | &lt;border-style&gt;{1,4} | Sets the style of the four borders (shorthand). | |
| | border-top | &lt;border-width&gt; &lt;border-style&gt; &lt;border-color&gt;	| Sets all the top border properties (shorthand). | |
| | border-top-color | &lt;color&gt; &#124; transparent | Sets the color of the top border. | |
| | border-top-style | none &#124; solid | Sets the style of the top border. | |
| | border-top-width | medium &#124; thin &#124; thick &#124; &lt;length&gt;	| Sets the width of the top border. | |
| | border-width | &lt;border-width&gt; | Sets the width of the four borders (shorthand). | |
| | border-image-source | &lt;image&gt; &#124; none | The path to the image is to be used as a border. | |
| | border-image-slice | &lt;number&gt; fill | How to slice the border image. &lt;number&gt; value can take only one value and initial value is 0 (not 100%). | |
| | border-image-width | &lt;length&gt; &#124; &lt;number&gt;	| Width of the border image. &lt;number&gt; value represents multiples of the corresponding border-top-width. | |
| [Display](https://www.w3.org/TR/CSS2/visuren.html#display-prop) | display | inline &#124; block &#124; inline-block &#124; table &#124; inline-table &#124; table-row-group &#124; table-header-group &#124; table-footer-group &#124; table-row &#124; table-column-group &#124; table-column &#124; table-cell &#124; table-caption &#124; none | The display property specifies the type of box used for an HTML element (Also check Visibility) |  |
| [Position](https://www.w3.org/TR/CSS2/visuren.html#positioning-scheme) | position | static &#124; absolute &#124; relative | The position property specifies the type of positioning method used for an element. | Each element in the document tree generates zero or more boxes according to the box model. The layout of these boxes is governed by box dimensions, type, positioning scheme, relationships between in the document tree and external information. \*CSS direction property only accepts "ltr" as a value. To support right-to-left text, the dir attribute in an HTML element should be used, e.g., &lt;html dir="rtl"&gt; (Also check Layers, Direction, Visual Formatting Model, and Visual Effects) |
| | top | &lt;length&gt; &#124; &lt;percentage&gt; &#124; auto | Sets the top edge of an element to a unit above/below the top edge of its nearest positioned ancestor. | |
| | right | &lt;length&gt; &#124; &lt;percentage&gt; &#124; auto | Sets the right edge of an element to a unit above/below the right edge of its nearest positioned ancestor. | |
| | bottom | &lt;length&gt; &#124; &lt;percentage&gt; &#124; auto | Sets the bottom edge of an element to a unit above/below the bottom edge of its nearest positioned ancestor. | |
| | left | &lt;length&gt; &#124; &lt;percentage&gt; &#124; auto | Sets the left edge of an element to a unit above/below the left edge of its nearest positioned ancestor. | |
| [Floats](https://www.w3.org/TR/2011/REC-CSS2-20110607/visuren.html#floats) | float | left &#124; right &#124; none | Specifies whether a box should float to the left, right, or not at all. | |
| | clear | none &#124; left &#124; right &#124; both | Indicates which sides of an element's box(es) may not be adjacent to an earlier floating box. | |
| [Layered presentation](https://www.w3.org/TR/2011/REC-CSS2-20110607/visuren.html#layers) | z-index | auto &#124; &lt;integer&gt; | Specifies the stack order of an element. | |
| [Text direction](https://www.w3.org/TR/2011/REC-CSS2-20110607/visuren.html#direction) | direction | ltr &#124; rtl | Specifies the text direction/writing direction. | |
| | unicode-bidi | normal &#124; embed | This property together with the direction property relates to the handling of bidirectional text in a document. | |
| [Width, height](https://www.w3.org/TR/2011/REC-CSS2-20110607/visudet.html#q10.0) | width | &lt;length&gt; &#124; &lt;percentage&gt; &#124; auto | Sets the width of an element. | |
| | min-width | &lt;length&gt; &#124; &lt;percentage&gt; | Sets the minimum width of an element. | |
| | max-width | &lt;length&gt; &#124; &lt;percentage&gt; &#124; none | Sets the maximum width of an element. | |
| | height | &lt;length&gt; &#124; &lt;percentage&gt; &#124; auto | Sets the height of an element. | |
| | min-height | 	&lt;length&gt; &#124; &lt;percentage&gt; | sets the minimum height of an element. | |
| | max-height | &lt;length&gt; &#124; &lt;percentage&gt; &#124; none | Sets the maximum height of an element. | |
| [Line height](https://www.w3.org/TR/2011/REC-CSS2-20110607/visudet.html#line-height) | line-height | normal &#124; &lt;number&gt; &#124; &lt;length&gt; &#124; &lt;percentage&gt; | Sets the line height. | |
| | vertical-align | baseline &#124; sub &#124; super &#124; top &#124; text-top &#124; middle &#124; bottom &#124; text-bottom &#124; &lt;length&gt; &#124; &lt;percentage&gt; | Sets the vertical alignment of an element. | |
| [Overflow](https://www.w3.org/TR/2011/REC-CSS2-20110607/visufx.html#overflow) | overflow | visible &#124; hidden | Specifies what happens if content overflows an element's box. | |
| [Visibility](https://www.w3.org/TR/2011/REC-CSS2-20110607/visufx.html#visibility) | visibility | visible &#124; hidden | Specifies whether or not an element should be visible | |
| [Generated content](https://www.w3.org/TR/2011/REC-CSS2-20110607/generate.html#content) | content | normal &#124; none &#124; [ &lt;string&gt; &#124; attr(&lt;identifier&gt;) ]+ | This property is used with the :before and :after pseudo-elements to generate content in a document. | |
| [Color](https://www.w3.org/TR/css3-color/) | color | &lt;color&gt; | Sets the color of text. HSL color value is not supported | CSS uses color-related properties and values to color the text, backgrounds, borders, and other parts of elements in a document. |
| | opacity | alpha value (0.0 ~ 1.0) | Sets the opacity level for an element | |
| [Background](https://www.w3.org/TR/CSS2/colors.html#background) | background | &lt;background-color&gt; &lt;background-image&gt; &lt;background-repeat&gt; | A shorthand property for setting all the background properties in one declaration | The background property sets all the background properties. (Also check Background) |
| | background-color | &lt;color&gt; &#124; transparent | Specifies the background color of an element. | |
| | background-image | &lt;uri&gt; &#124; none | Specifies one or more background images for an element. Multiple layering is not supported. | |
| | background-position | [ [ &lt;percentage&gt; &#124; &lt;length&gt; &#124; left &#124; center &#124; right ] [ &lt;percentage&gt; &#124; &lt;length&gt; &#124; top &#124; center &#124; bottom ]? ] &#124; [ [ left &#124; center &#124; right ] &#124;&#124; [ top &#124; center &#124; bottom ] ] | Sets the initial position for each defined background image, relative to the background position layer defined by background-origin. | |
| | background-position-x | [ center &#124; [ left &#124; right ]? &lt;length-percentage&gt; ] | Sets the initial horizontal position, relative to the background position layer defined by background-origin for each defined background image. | |
| | background-position-y | [ center &#124; [ left &#124; right ]? &lt;length-percentage&gt; ] | Sets the initial vertical position, relative to the background position layer defined by background-origin for each defined background image. | |
| | background-repeat | repeat &#124; repeat-x &#124; repeat-y &#124; no-repeat | Sets how a background image will be repeated | |
| | background-size	| &lt;length&gt; &#124; &lt;percentage&gt; &#124; auto &#124; cover &#124; contain | Specifies the size of the background image(s). | |
| [Font](https://www.w3.org/TR/CSS2/fonts.html) | font-style | normal &#124; italic &#124; oblique | Specifies the font style for text. | A font provides a resource containing the visual representation of characters. |
| | font-weight | normal &#124; bold &#124; bolder &#124; lighter &#124; 100 &#124; 200 &#124; 300 &#124; 400 &#124; 500 &#124; 600 &#124; 700 &#124; 800 &#124; 900 | Specifies the weight of a font. | |
| | font-size | &lt;absolute-size&gt; &#124; &lt;relative-size&gt; &#124; &lt;length&gt; &#124; &lt;percentage&gt; | Specifies the font size of text. | Possible values of an &lt;absolute-size&gt; keyword: [ xx-small &#124; x-small &#124; small &#124; medium &#124; large &#124; x-large &#124; xx-large ] <br> Possible values of an &lt;relative-size&gt; keyword: [ larger &#124; smaller] |
| [Text](https://www.w3.org/TR/CSS2/text.html) | text-align | left &#124; right &#124; center | Specifies the horizontal alignment of text in an element | This CSS3 module defines properties for text manipulation and specifies their processing model. It covers line breaking, justification and alignment, white space handling, and text transformation. |
| | text-decoration | none &#124; [ underline &#124;&#124; line-through ] | Specifies the decoration added to the text | |
| | white-space | normal &#124; pre &#124; nowrap &#124; pre-wrap &#124; pre-line | Describes how whitespace inside the element is handled. | |
| [Table](https://www.w3.org/TR/2011/REC-CSS2-20110607/tables.html#q17.0) | table-layout | fixed &#124; auto | Defines the algorithm to be used to lay out table cells, rows, and columns. | |
| | caption-side | 	top &#124; bottom | Positions the content of a table's &lt;caption&gt; on the specified side. | |
| | border-spacing | 	&lt;length&gt; &lt;length&gt;? | Specifies the distance between the borders of adjacent table cells (only for the separated borders model). | |
| [Transform](https://www.w3.org/TR/css-transforms-1/) | transform | none &#124; matrix &#124; translate &#124; translateX &#124; translateY &#124; scale &#124; scaleX &#124; scaleY &#124; rotate &#124; skew &#124; skewX &#124; skewY | Applies a 2D transformation to an element. | The transform property applies a 2D transformation to an element. This property allows you to rotate, scale, move and skew. A transformable element is an element whose layout is governed by the CSS box model which is either a block-level or atomic inline-level element. |
| | transform-origin | &lt;percentage&gt; &#124; &lt;length&gt; &#124; top &#124; right &#124; bottom &#124; left &#124; center | Changes the position of transformed elements | |
| [Media Queries - Media Types](https://www.w3.org/TR/css3-mediaqueries/) | all &#124; screen | all &#124; screen | Describes media types supported by web widget engine. | ‘all’ means suitable for all supported devices. |
| [Media Queries - Media Features](https://www.w3.org/TR/css3-mediaqueries/#media1) | width | &lt;length&gt; | Describes the width of the targeted display area of the output device. | |
| | height | &lt;length&gt; | Describes the height of the targeted display area of the output device. | |
| | device-width | &lt;length&gt; | Describes the width of the rendering surface of the output device. | |
| | device-heigth | &lt;length&gt; | Describes the height of the rendering surface of the output device. | |
| | orientation | portrait &#124; landscape| ‘portrait’ when the value of the ‘height’ media feature is greater than or equal to the value of the ‘width’ media feature. Otherwise ‘orientation’ is ‘landscape’. | |
| | aspect-ratio | &lt;ratio&gt; | The ratio of the value of the ‘width’ media feature to the value of the ‘height’ media feature. | |
| | device-aspect-ratio | &lt;ratio&gt; | The ratio of the value of the ‘device-width’ media feature to the value of the ‘device-height’ media feature. | |
| | color | &lt;integer&gt; | Describes the number of bits per color component of the output device. If the device is not a color device, the value is zero. | |
| | color-index | &lt;integer&gt; | Describes the number of entries in the color lookup table of the output device. If the device does not use a color lookup table, the value is zero. | |
| | monochrome | &lt;integer&gt; | Describes the number of bits per pixel in a monochrome frame buffer. If the device is not a monochrome device, the output device value will be 0. | |
| | resolution | &lt;resolution&gt; | Describes the resolution of the output device, i.e. the density of the pixels.  | |
| | scan | progressive &#124; interlace | Describes the scanning process of "tv" output devices. | The Web widget engine doesn't support this feature. |
| | grid | &lt;integer&gt; | This is used to query whether the output device is grid or bitmap. If the output device is grid-based (e.g., a "tty" terminal, or a phone display with only one fixed font), the value will be 1. Otherwise, the value will be 0. | The Web widget engine supports only bitmap device. |


## Selectors

| Selectors | Type | Pattern | Usage | Description |
|-----------|------|---------|-------|-------------|
| [Selectors](https://www.w3.org/TR/selectors/) | Universal Selector | * | * | Selects all elements |
| | Type Selector | element | p | Selects all \<p\> elements. The 'OR' condition is allowed (e.g., element, element) |
| | Class Selector | element.class | div.intro | Selects all \<div\> elements with class="intro". A subset matching of "class" values is not allowed (for example, div.class1.class2) |
| | ID Selector | element#id | div#firstname | Selects an \<div\> element with id="firstname" |
| | Attribute Selectors | [attr] | [target] | Selects all elements with a target attribute |
| | | [attr=val] | [lang=en] | Selects all elements with lang="en" |
| | | [att~=val] | [title~=flower] | Selects all elements with a title attribute containing the word "flower" |
| | | [att&#124;=val] | [lang&#124;=en] | Selects all elements with a lang attribute value starting with "en" |
| | | [att^=val] | a[href^="https"] | Selects every \<a\> element whose href attribute value begins with "https" |
| | | [att$=val] | a[href$=".pdf"] | Selects every \<a\> element whose href attribute value ends with ".pdf" |
| | | [att*=val] | a[href*="w3schools"] | Selects every \<a\> element whose href attribute value contains the substring "w3schools" |
| | Pseudo-classes | :hover | a:hover | Selects links on mouse over |
| | | :active | a:active | Selects the active link |
| | | :focus | input:focus | Selects the input element which has focus |
| | | :target | #news:target | Selects the current active #news element (clicked on a URL containing that anchor name) |
| | | :lang(language) | p:lang(it) | Selects every \<p\> element with a lang attribute equal to "it" (Italian) |
| | | :root | :root | Selects the document's root element |
| | | :nth-child(n) | p:nth-child(2) | Selects every \<p\> element that is the second child of its parent |
| | | :nth-last-child(n) | p:nth-last-child(2) | Selects every \<p\> element that is the second child of its parent, counting from the last child |
| | | :nth-of-type(n) | p:nth-of-type(2) | Selects every \<p\> element that is the second \<p\> element of its parent |
| | | :nth-last-of-type(n) | p:nth-last-of-type(2) | 	Selects every \<p\> element that is the second \<p\> element of its parent, counting from the last child |
| | | :first-child | p:first-child | Selects every \<p\> element that is the first child of its parent |
| | | :last-child | p:last-child | Selects every \<p\> element that is the last child of its parent |
| | | :first-of-type | p:first-of-type | Selects every \<p\> element that is the first \<p\> element of its parent |
| | | :last-of-type | p:last-of-type | Selects every \<p\> element that is the last \<p\> element of its parent |
| | | :only-child | p:only-child | Selects every \<p\> element that is the only child of its parent |
| | | :only-of-type | p:only-of-type | Selects every \<p\> element that is the only \<p\> element of its parent |
| | Pseudo-elements | ::first-line | p::first-line | Selects the first line of every \<p\> element|
| | | ::first-letter | p::first-letter | Selects the first letter of every \<p\> element |
| | | ::before | p::before | Insert something before the content of each \<p\> element |
| | | ::after | p::after | Insert something after the content of each \<p\> element |
| | Combinators | selector1 selector2 | div p | Selects all \<p\> elements inside \<div\> elements |
| | | selector1 > selector2 | div > p | Selects all \<p\> elements that are immediate children of a \<div\> element |
| | | selector1 + selector2 | div + p | Selects all \<p\> elements that are placed immediately after \<div\> elements |
| | | selector1 ~ selector2 | div ~ p | Selects all \<p\> elements that are siblings of \<div\> elements |

## Additional Supported APIs

### XMLHttpRequest
XMLHttpRequest is a constructor object. It is created by a `new` command, e.g., `var xhr = new XMLHttpRequest();` In addition, two XHR objects are created and executed concurrently by the threadpool by default.

| Interface            | Type   | Name                      | Description |
|----------------------|--------|---------------------------|-------------|
| [XMLHttpRequestEventTarget](https://xhr.spec.whatwg.org/#xmlhttprequesteventtarget) | enum        | XMLHttpRequestResponseType  | "", "text", "arraybuffer", "document", "blob", "json"|
| | attribute	| onloadstart	| Function called when the request starts. Usage: onloadstart: function() {} |
| | attribute	| onprogress	| Function called when transmitting data. Usage: onprogress: function() {} |
| | attribute	| onabort	| Function called when the request has been aborted. For instance, by invoking the abort() method. Usage: onabort: function() {} |
| | attribute	| onerror	| Function called when the request has failed. Usage: onerror: function() {} |
| | attribute	| onload	| Function called when the request has successfully completed. Usage: onload: function() {} |
| | attribute	| ontimeout	| Function called when the author specified timeout has passed before the request completed. Usage: ontimeout: function() {} |
| | attribute	| onloadend	| Function called when the request has completed (either in success or failure). Usage: onloadend: function() {} |
| [XMLHttpRequest](https://xhr.spec.whatwg.org/#xmlhttprequest) | constructor | XMLHttpRequest() |  |
| | attribute    | onReadyStateChange | The readyState attribute changes value, except when it changes to UNSENT. Usage: onreadystatechange: function() {} |
| | attribute	| timeout	| Can be set to a time in milliseconds.Terminates fetching after the given time (in milliseconds) has passed. If the fetching has not completed after the time passed and the synchronous flag is unset, a timeout event will be dispatched. |
| | attribute	| status	| Returns 0 if the state is UNSENT or OPENED, or error flag is set. Otherwise returns the HTTP status code.|
| | attribute	| responseType	| Sets or returns the response type, which is either "", "blob", "json", or "text".|
| | attribute	| response	| Returns the response entity body, which is either string, Blob object, object, or string when responseType is "", "blob", "json", or "text", respectively.|
| | attribute	| responseText	| Returns an empty string if the state is not LOADING or DONE, or error flag is set. Returns the text response entity body when responseType is either "" or "text". The allowed character set for response text is UTF-8. Otherwise returns an invalidStateError exception with either "Permission denied", "Position unavailable", or "Timeout expired".|
| | attribute	| readyState*	| Returns the current state, which is one of the readyState code shown below.|
| | method    | void open(ByteString method, DOMString url, boolean async = true, optional DOMString? username = null, optional DOMString? password = null)    | Sets the request method, request URL, and synchronous flag. Supported request method : GET, POST |
| | method    | void setRequestHeader(ByteString name, ByteString value)    | Combines a header in author request headers. |
| | method    | void send(optional DOMString? body = null)    | Initiates the request. The optional 'data' argument allows only UTF-8 encoded string type. The argument is ignored if request method is GET. |
| | method    | void abort();    | Cancels any network activity. |


\* The readyState code are as follows.

| readyStateCode | Description | Numeric Value |
|----------------|-------------|---------------|
| UNSENT         | The object has been constructed. | 0 |
| OPENED         | The open() method has been successfully invoked. | 1 |
| HEADERS_RECEIVED | All redirects (if any) have been followed and all HTTP headers of the final response have been received. | 2 |
| LOADING        | 	The response entity body is being received. | 3 |
| DONE           | The data transfer has been completed or something went wrong during the transfer (for example, infinite redirects). | 4 |


### Blob
Blob object is used by an XMLHTTPRequest object to retrieve binary data. Supported binary data are the resources supported by the widget engine. When blob is used for other types of binary data, it is likely that the binary data is not recognized by the widget engine.

| Interface            | Type   | Name                      | Description |
|----------------------|--------|---------------------------|-------------|
| [Blob](https://w3c.github.io/FileAPI/#blob) | interface | Blob | A Blob object refers to a byte sequence |
| | constructor | Blob(optional sequence\<BlobPart\> blobParts = []) | |
| |	attribute |	size    | Returns the size of the byte sequence in number of bytes |
| |	attribute |	type	| The ASCII-encoded string in lower case representing the media type of the Blob |
| |	method	| Blob slice([Clamp] optional long long start = 0, [Clamp] optional long long end = size, optional DOMString contentType = "")	| Returns a new Blob object with bytes ranging from the optional start parameter up to but not including the optional end parameter, and with a type attribute that is the value of the optional contentType parameter. It must act as follows: |
| |	typedef | (BufferSource or Blob or DOMString) BlobPart | |


### Geolocation
Extensions to the Navigator Object: The navigator is extended by the following attributes and methods.

| Interface            | Type   | Name                      | Description |
|----------------------|--------|---------------------------|-------------|
| [Navigator](https://html.spec.whatwg.org/#the-navigator-object)	| interface	| Navigator	| The navigator attribute of the Window interface must return an instance of the Navigator interface, which represents the identity and state of the user agent (the client), and allows Web pages to register themselves as potential protocol and content handlers |
| |	attribute	| geolocation	| Return geolocation interface |
| [NavigatorID](https://html.spec.whatwg.org/multipage/#navigatorid)	| interface	|	| NavigatorID is used for identifying Navigator |
| |	attribute	| appCodeName	| Returns  the string "StarFish".|
| |	attribute	| appName	| Returns  the string "StarFish".|
| |	attribute	| appVersion	| Returns the string "Mozilla/5.0 StarFish/0.1".|
| |	attribute	| userAgent	| Returns the string "Mozilla/5.0 StarFish/0.1".|
| |	attribute	| vendor	| Returns the string "Samsung Electronics Co., Ltd."|
| [Geolocation](https://dev.w3.org/geo/api/spec-source.html#geolocation) | interface	| Geolocation | |
| | method   | void getCurrentPosition(PositionCallback successCallback, optional PositionErrorCallback errorCallback, optional PositionOptions options)	| Parameters are in following formats:<br>`successCallback`: `function(position) {}`<br>`errorCallback`: `function (positionError) {}`<br>`options`: `PositionOptions` |
| | callback | PositionCallback = void (Position position) | |
| | callback | PositionErrorCallback = void (PositionError positionError) | |
| [Coordinates](https://dev.w3.org/geo/api/spec-source.html#coordinates_interface) | attribute | latitude | The latitude attribute is a geographic coordinate specified in decimal degrees. |
| | attribute | longitude | The longitude attribute is a geographic coordinate specified in decimal degrees. |
| | attribute |	altitude | The altitude attribute denotes the height of the position, specified in meters above the WGS84 ellipsoid.|
| | attribute | accuracy | The accuracy attribute denotes the accuracy level of the latitude and longitude coordinates. It is specified in meters, and is a non-negative real number.|
| | attribute |  altitudeAccuracy |	 Not supported by the widget engine. Always returns null.|
| | attribute |	 heading | The heading attribute denotes the direction of travel of the hosting device and is specified in degrees, where 0° ≤ heading < 360°, counting clockwise relative to the true north.|
| | attribute |	 speed | The speed attribute denotes the magnitude of the horizontal component of the hosting device's current velocity and is specified in meters per second. The value of the speed attribute is a non-negative real number.|
| [Geoposition](none)	| interface	| Geoposition	| The Geoposition interface represents the position of the concerned device at a given time |
| | attribute	| coords | Returns a Coordinates object defining the current location. |
| |	attribute	| timestamp | Returns a DOMTimeStamp representing the time at which the location was retrieved. |
| [PositionError](https://dev.w3.org/geo/api/spec-source.html#position_error_interface) | attribute | code* |	 Returns the appropriate position error code |
| | message |	Returns an error message describing the details of the error encountered. | |
| |	constant |	PERMISSION_DENIED = 1 | |
| | constant |	POSITION_UNAVAILABLE = 2 | |
| | constant | TIMEOUT = 3 | | |


\* PositionError codes are as follows:

| Error Code | Description | Numeric Value |
|------------|-------------|---------------|
| PERMISSION_DENIED | The location acquisition process failed because the widget does not have permission to use the Geolocation API. | 1 |
| POSITION_UNAVAILABLE | The position of the device could not be determined. | 2 |
| TIMEOUT | The length of time specified by the timeout property has elapsed before successfully acquiring a new Position object. | 3 |

## Web Device API
The following describes Web device APIs supported by Widget Engine. Supported interfaces and methods are generally the same as the interfaces and methods supported by Tizen API, respectively. If there are exceptions, they are explicitly mentioned below.

| API            | Description | Note |
|----------------|-------------|------|
| [Application](https://developer.tizen.org/dev-guide/2.3.1/org.tizen.web.apireference/html/device_api/wearable/tizen/application.html) | This API allows a widget to launch and access installed applications. Note that launching other Web widgets installed on the same device is deprecated in Tizen 3.0, so such use is not recommended. | Unsupported methods:<br>- ApplicationManager: Application getCurrentApplication()<br>- Application: void exit()<br>- Application: void hide()<br>- Application: RequestedApplicationControl getRequestedAppControl() |
| [Preference](https://developer.tizen.org/development/api-references/web-application?redirect=/dev-guide/2.3.2/org.tizen.web.apireference/html/device_api/wearable/tizen/preference.html) | This API allows to store and retrieve a (key, value) pair to set application preferences. | |
| [Sensor](https://developer.tizen.org/development/api-references/web-application?redirect=/dev-guide/2.3.1/org.tizen.web.apireference/html/device_api/wearable/tizen/sensor.html) | This API provides interfaces and methods for getting sensor data from the various device sensors. | |
| [System Information](https://developer.tizen.org/development/api-references/web-application?redirect=/dev-guide/2.3.1/org.tizen.web.apireference/html/device_api/wearable/tizen/systeminfo.html) | This API provides information about the device's display, network, storage and other capabilities. | Unsupported feature:<br>Getting 'LOCALE' value using addPropertyValueChangeListener() is not supported. Because when 'LOCALE' is changed, web widget will be restarted so it is not possible to get 'LOCALE' at runtime. |
| [SAP*](http://img-developer.samsung.com/onlinedocs/samsung_webapi_guide_public_2.0/html/wapi_spec/sap.html) | The Samsung Accessory Protocol (SAP) APIs offers services that enable mobile devices to establish connections and exchange data with web widget applications. | |
| [Widgetservice](https://developer.tizen.org/development/api-references/web-application?redirect=/dev-guide/2.3.2/org.tizen.web.apireference/html/device_api/wearable/tizen/widgetservice.html) | This API provides information about installed widgets. | Unsupported methods:<br>- WidgetInstance: void changeUpdatePeriod()<br>- WidgetInstance: void sendContent()<br>- WidgetInstance: void getContent()<br>Important Notice<br>- The `WidgetInstanceId` in interface `WidgetInstance` is not persistent, i.e., each widget instance is assigned a new `WidgetInstace.id` at every reboot of a wearable device. Therefore, it cannot be used to identify a widget instance existed before rebooting a device. |

\* The SAP (Samsung Accessory Protocol) is a Samsung device API.
