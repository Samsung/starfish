# Specification

## HTML
This section describes the complete list of supported HTML tags and attributes by the Web widget engine. Please note that only the tags and attributes mentioned explicitly in this section are supported. In addition, the Widget engine supports only HTML5 documents, and it assumes all input documents are HTML5 documents even if `!DOCTYPE` is not explicitly specified.

| HTML Tag | Attribute | Allowed Value | Usage | Note |
|----------|-----------|---------------|-------|------|
| [Global Attribute](https://www.w3.org/TR/html5/dom.html#global-attributes) | class | &lt;string&gt; | &lt;element class="classname"&gt; | &lt;css_styles&gt; must conform to the widget engine's [CSS](https://developer.tizen.org/development/api-references/web-application?redirect=https%3A//developer.tizen.org/dev-guide/2.3.2/org.tizen.web.apireference/html/widget_spec/web_widget.html&langredirect=1#user-content-css) rules |
| | dir | ltr &#124; rtl | &lt;element dir="ltr"&gt; | |
| | id | &lt;string&gt; | &lt;element id="id"&gt; | |
| | style | &lt;css_styles&gt; | &lt;element style="css_styles"&gt; | |
| [html](https://www.w3.org/TR/html5/semantics.html#the-root-element) | | | | |
| [head](https://www.w3.org/TR/html5/document-metadata.html#the-head-element) | | | | |
| [link](https://www.w3.org/TR/html5/document-metadata.html#the-link-element) | rel | stylesheet | &lt;link rel="stylesheet"&gt; | &lt;URL&gt; must be a local path |
| | href | &lt;URL&gt; | &lt;link href="local_path"&gt; | |
| | type | text/css | &lt;link tyle="text/css"&gt; | |
| [meta](https://www.w3.org/TR/html5/document-metadata.html#the-meta-element) | charset | UTF-8 | &lt;meta charset="UTF-8"&gt; | `name` and `content` are used to set the widget background transparent only. To do so, both `name` and `content` must be set in the same `meta` tag |
| | name | tizen-widget-transparent-background | &lt;meta name="tizen-widget-transparent-background" content="yes"&gt; | |
| | content | yes &#124; no | &lt;meta name="tizen-widget-transparent-background" content="yes"&gt; | |
| [style](https://www.w3.org/TR/html5/document-metadata.html#the-style-element) | type | text/css | &lt;style type="text/css"&gt; | &lt;URL&gt; must be a local path |
| [body](https://www.w3.org/TR/html5/sections.html#the-body-element) | | | | |
| [h1, h2, h3, h4, h5, and h6](https://www.w3.org/TR/html5/sections.html#the-h1,-h2,-h3,-h4,-h5,-and-h6-elements) | | | | |
| [p](https://www.w3.org/TR/html5/grouping-content.html#the-p-element) | | | | |
| [div](https://www.w3.org/TR/html5/grouping-content.html#the-div-element) | | | | |
| [span](https://www.w3.org/TR/html5/text-level-semantics.html#the-span-element) | | | | |
| [br](https://www.w3.org/TR/html5/text-level-semantics.html#the-br-element) | | | | |
| [image](https://www.w3.org/TR/html5/embedded-content-0.html#the-img-element) | src | &lt;URL&gt; | &lt;img src="local_path"&gt; | &lt;URL&gt; must be a local path. Supported images are of type `.png`, `.jpg`, and `.bmp` |
| | height | pixels | &lt;img height="pixels"&gt; | |
| | width | pixels | &lt;img width="pixels"&gt; | |
| [script](https://developer.tizen.org/development/api-references/web-application?redirect=https%3A//developer.tizen.org/dev-guide/2.3.2/org.tizen.web.apireference/html/widget_spec/web_widget.html&langredirect=1#user-content-additional-supported-apis) | src | &lt;URL&gt; | &lt;script src="local_path"&gt; | &lt;URL&gt; must be a local path |
| | type | text/javascript | &lt;script type="text/javascript"&gt; | |
| | charset | UTF-8 | &lt;script charset="UTF-8"&gt; | |
| [table](https://www.w3.org/TR/html5/tabular-data.html#the-table-element) | width | pixels &#124; &lt;percentage&gt; | | |
| | bgcolor | &lt;color&gt; | | |
| [caption](https://www.w3.org/TR/html5/tabular-data.html#the-caption-element) | | | | |
| [colgroup](https://www.w3.org/TR/html5/tabular-data.html#the-colgroup-element) | | | | |
| [col](https://www.w3.org/TR/html5/tabular-data.html#the-col-element) | | | | |
| [tbody](https://www.w3.org/TR/html5/tabular-data.html#the-tbody-element) | | | | |
| [thead](https://www.w3.org/TR/html5/tabular-data.html#the-thead-element) | | | | |
| [tfoot](https://www.w3.org/TR/html5/tabular-data.html#the-tfoot-element) | | | | |
| [tr](https://www.w3.org/TR/html5/tabular-data.html#the-tr-element) | bgcolor | &lt;color&gt; | | |
| [td](https://www.w3.org/TR/html5/tabular-data.html#the-td-element) | width | pixels &#124; &lt;percentage&gt; | | |
| | colspan | number | | |
| | bgcolor | &lt;color&gt; | | |
| [th](https://www.w3.org/TR/html5/tabular-data.html#the-th-element) | width | pixels &#124; &lt;percentage&gt; | | |
| | bgcolor | &lt;color&gt; | | |
| [video](https://www.w3.org/TR/html5/embedded-content-0.html#the-video-element) | | | | |
| [DOCTYPE](https://www.w3.org/TR/html5/syntax.html#the-doctype) | | html | &lt;!DOCTYPE html&gt; | The DOCTYPE declaration must be the first tag in your HTML document. The Web widget engine supports HTML5 only. Other versions of HTMLs and HTML modes (such as quirks mode) are not supported. |


## DOM

| Interface            | Type   | Name                      | Description |
|----------------------|--------|---------------------------|-------------|
| [NonElementParentNode](https://www.w3.org/TR/domcore/#interface-nonelementparentnode) | method | getElementById(elementId) | Returns the first element within node's descendants whose ID is elementId |
| [ParentNode](https://www.w3.org/TR/domcore/#interface-parentnode) | attribute | children | Returns child elements |
| | | firstElementChild | Returns the first child element, or null otherwise |

## Event
| Interface | Type | Name | Description | Note |
|-----------|------|------|-------------|------|
| [Global event handler attribute](https://www.w3.org/TR/html5/webappapis.html#event-handler-attributes) | attribute | onclick | The onclick event occurs when the user clicks on an element | Attributes that are common to all elements in the HTML languages. |
| |  | onload | The onload event occurs when an object has been loaded | |

## CSS

| Type | Property | Allowed Value | Description | Note |
|------|----------|---------------|-------------|------|
| [Margin](https://www.w3.org/TR/CSS2/box.html#margin-properties) | margin | \<margin-width\>{1,4} | The margin shorthand property sets all the margin properties in one declaration | The margin properties specify the width of the margin area of a box. \<margin-width\> may take one of the following values: \<length\>, \<percentage\>, auto. (Also check [Length](https://www.w3.org/TR/CSS2/syndata.html#length-units)) |
| | margin-bottom | \<margin-width\> &#124; \<percentage\> &#124; auto | Sets the bottom margin of an element | |
| | margin-left	| \<length\> &#124; \<percentage\> &#124; auto | Sets the left margin of an element | |
| | margin-right | \<length\> &#124; \<percentage\> &#124; auto | Sets the right margin of an element | |
| | margin-top | \<length\> &#124; \<percentage\> &#124; auto | Sets the top margin of an element | |
| [Padding](https://www.w3.org/TR/CSS2/box.html#padding-properties) | padding | \<padding-width\>{1,4} | The padding shorthand property sets all the padding properties in one declaration | \<padding-width\> may take one of the following values: \<length\>, \<percentage\> |
| | padding-bottom | \<length\> &#124; \<percentage\>	| Sets the bottom padding for an element | |
| | padding-left | \<length\> &#124; \<percentage\>	| Sets the left padding for an element | |
| | padding-right | \<length\> &#124; \<percentage\> | Sets the right padding for an element | |
| | padding-top | \<length\> &#124; \<percentage\> | Sets the top padding for an element | |
| [Border](https://www.w3.org/TR/css3-border/) | border | \<border-width\> \<border-style\> \<border-color\> | Sets all the border properties (shorthand). | The border can either be a predefined style (solid line) or it can be an image. In the former case, various properties define the style (\<border-style\>), color (\<border-color\>), and thickness (\<border-width\>) of the border. \<border-width\> may take one of the following values: thin, medium, thick, \<length\> \<border-color\> may take one of the following values: \<color\>, transparent \<border-style\> may take one of the following values: none, solid (Also check Border Properties) |
| | border-bottom | \<border-width\>   \<border-style\>   \<border-color\> | Sets all the bottom border properties (shorthand). | |
| | border-bottom-color | \<color\> &#124; transparent | Sets the color of the bottom border. | |
| | border-bottom-style | none &#124; solid | Sets the style of the bottom border. | |
| | border-bottom-width | medium &#124; thin &#124; thick &#124; \<length\> | Sets the width of the bottom border. | |
| | border-color | \<border-color\>{1,4} | Sets the color of the four borders (shorthand). | |
| | border-left | \<border-width\>   \<border-style\>   \<border-color\> | Sets all the left border properties (shorthand). | |
| | border-left-color | \<color\> &#124; transparent | Sets the color of the left border. | |
| | border-left-style | none &#124; solid | Sets the style of the left border. | |
| | border-left-width | medium &#124; thin &#124; thick &#124; \<length\> | Sets the width of the left border. | |
| | border-right | \<border-width\>   \<border-style\>   \<border-color\>	| Sets all the right border properties (shorthand). | |
| | border-right-color | \<color\> &#124; transparent | Sets the color of the right border. | |
| | border-right-style | none &#124; solid | Sets the style of the right border. | |
| | border-right-width | medium &#124; thin &#124; thick &#124; \<length\>	| Sets the width of the left border. | |
| | border-style | \<border-style\>{1,4} | Sets the style of the four borders (shorthand). | |
| | border-top | \<border-width\> \<border-style\> \<border-color\>	| Sets all the top border properties (shorthand). | |
| | border-top-color | \<color\> &#124; transparent | Sets the color of the top border. | |
| | border-top-style | none &#124; solid | Sets the style of the top border. | |
| | border-top-width | medium &#124; thin &#124; thick &#124; \<length\>	| Sets the width of the top border. | |
| | border-width | \<border-width\> | Sets the width of the four borders (shorthand). | |
| | border-image-source | \<image\> &#124; none | The path to the image is to be used as a border. | |
| | border-image-slice | \<number\> fill | How to slice the border image. \<number\> value can take only one value and initial value is 0 (not 100%). | |
| | border-image-width | \<length\> &#124; \<number\>	| Width of the border image. \<number\> value represents multiples of the corresponding border-top-width. | |
| [Display](https://www.w3.org/TR/CSS2/visuren.html#display-prop) | display | &#124; block &#124; inline &#124; inline-block &#124; none | Displays elements as inline elements | The display property specifies the type of box used for an HTML element (Also check Visibility) |
| | visibility | visible &#124; hidden | Specifies whether or not an element should be visible | |
| [Position](https://www.w3.org/TR/CSS2/visuren.html#positioning-scheme) | position | static &#124; absolute &#124; relative | The position property specifies the type of positioning method used for an element. | Each element in the document tree generates zero or more boxes according to the box model. The layout of these boxes is governed by box dimensions, type, positioning scheme, relationships between in the document tree and external information. \*CSS direction property only accepts "ltr" as a value. To support right-to-left text, the dir attribute in an HTML element should be used, e.g., \<html dir="rtl"\> (Also check Layers, Direction, Visual Formatting Model, and Visual Effects) |
| | bottom | \<length\> &#124; \<percentage\> &#124; auto | Sets the bottom edge of an element to a unit above/below the bottom edge of its nearest positioned ancestor. | |
| | content | FILL THIS ONE! | | |
| | height | \<length\> &#124; \<percentage\> &#124; auto | Sets the height of an element. | |
| | left | \<length\> &#124; \<percentage\> &#124; auto | Sets the left edge of an element to a unit above/below the left edge of its nearest positioned ancestor. | |
| | line-height | normal &#124; \<number\> &#124; \<length\> &#124; \<percentage\> | Sets the line height. | |
| | max-width | \<length\> &#124; \<percentage\> &#124; none | | |
| | min-width | \<length\> &#124; \<percentage\> | | |
| | max-height | \<length\> &#124; \<percentage\> &#124; none | | |
| | min-height | 	\<length\> &#124; \<percentage\> | | |
| | right | \<length\> &#124; \<percentage\> &#124; auto | Sets the right edge of an element to a unit above/below the right edge of its nearest positioned ancestor. | |
| | top	| \<length\> &#124; \<percentage\> &#124; auto | Sets the top edge of an element to a unit above/below the top edge of its nearest positioned ancestor. | |
| | vertical-align | baseline &#124; sub &#124; super &#124; top &#124; text-top &#124; middle &#124; bottom &#124; text-bottom &#124; \<length\> &#124; \<percentage\> | Sets the vertical alignment of an element. | |
| | white-space | FILL THIS ONE! | | |
| | width | \<length\> &#124; \<percentage\> &#124; auto | Sets the width of an element. | |
| | z-index | auto &#124; \<integer\> | Specifies the stack order of an element. | |
| | direction | ltr | Specifies the text direction/writing direction. | |
| | overflow | visible &#124; hidden | Specifies what happens if content overflows an element's box. | |
| [Color](https://www.w3.org/TR/css3-color/) | color | \<color\> | Sets the color of text. HSL color value is not supported | CSS uses color-related properties and values to color the text, backgrounds, borders, and other parts of elements in a document. |
| | opacity | alpha value (0.0 ~ 1.0) | Sets the opacity level for an element | |
| | clear | FILL THIS ONE! | | |
| | float | FILL THIS ONE! | | |
| [Background](https://www.w3.org/TR/CSS2/colors.html#background) | background | \<background-color\> \<background-image\> \<background-repeat\> | A shorthand property for setting all the background properties in one declaration | The background property sets all the background properties. (Also check Background) |
| | background-color | \<color\> &#124; transparent | Specifies the background color of an element. | |
| | background-image | \<uri\> &#124; none | Specifies one or more background images for an element. Multiple layering is not supported. | |
| | background-position | FILL THIS ONE! | | |
| | background-position-x | FILL THIS ONE! | | |
| | background-position-y | FILL THIS ONE! | | |
| | background-repeat | repeat &#124; repeat-x &#124; repeat-y &#124; no-repeat | Sets how a background image will be repeated | |
| | background-size	| \<length\> &#124; \<percentage\> &#124; auto &#124; cover &#124; contain | Specifies the size of the background image(s). | |
| [Font](https://www.w3.org/TR/CSS2/fonts.html) | font-style | normal &#124; italic &#124; oblique | Specifies the font style for text. | A font provides a resource containing the visual representation of characters. |
| | font-weight | normal &#124; bold &#124; bolder &#124; lighter &#124; 100 &#124; 200 &#124; 300 &#124; 400 &#124; 500 &#124; 600 &#124; 700 &#124; 800 &#124; 900 | Specifies the weight of a font. | |
| | font-size | medium &#124; xx-small &#124; x-small &#124; small &#124; large &#124; x-large &#124; xx-large &#124; smaller &#124; larger &#124; \<length\> &#124; \<percentage\> | Specifies the font size of text. | |
| [Text](https://www.w3.org/TR/CSS2/text.html) | text-align | left &#124; right &#124; center | Specifies the horizontal alignment of text in an element | This CSS3 module defines properties for text manipulation and specifies their processing model. It covers line breaking, justification and alignment, white space handling, and text transformation. |
| | text-decoration | none &#124; underline &#124; line-through | Specifies the decoration added to the text | |
| [Transform](https://www.w3.org/TR/css-transforms-1/) | transform | none &#124; matrix &#124; translate &#124; translateX &#124; translateY &#124; scale &#124; scaleX &#124; scaleY &#124; rotate &#124; skew &#124; skewX &#124; skewY | Applies a 2D transformation to an element. | The transform property applies a 2D transformation to an element. This property allows you to rotate, scale, move and skew. A transformable element is an element whose layout is governed by the CSS box model which is either a block-level or atomic inline-level element. |
| | transform-origin | \<percentage\> &#124; \<length\> &#124; top &#124; right &#124; bottom &#124; left &#124; center | Changes the position of transformed elements | |
| [Table](https://www.w3.org/TR/2011/REC-CSS2-20110607/tables.html#q17.0) | table-layout | fixed &#124; auto | | |
| | caption-side | 	top &#124; bottom | | |
| | border-spacing | 	\<length\> \<length\>? | | |
| | cellspacing | \<length\> | | |

## Selectors

| Selectors | Type | Pattern | Usage | Description |
|-----------|------|---------|-------|-------------|
| [Selectors](https://www.w3.org/TR/CSS2/selector.html) | Universal Selector | * | * | Selects all elements |
| | Type Selector | element | p | Selects all \<p\> elements. The 'OR' condition is allowed (e.g., element, element) |

## Additional Supported APIs

### XMLHttpRequest
XMLHttpRequest is a constructor object. It is created by a `new` command, e.g., `var xhr = new XMLHttpRequest();` In addition, two XHR objects are created and executed concurrently by the threadpool by default.

| Interface            | Type   | Name                      | Description |
|----------------------|--------|---------------------------|-------------|
| [XMLHttpRequestEventTarget](https://xhr.spec.whatwg.org/#xmlhttprequesteventtarget) | attribute | onloadstart | Function called when the request starts. Usage: `onloadstart: function() {}` |
| | | onprogress | Function called when transmitting data. Usage: `onprogress: function() {}` |
| | | onabort | Function called when the request has been aborted. For instance, by invoking the abort() method. Usage: `onabort: function() {}` |
| | | onerror | Function called when the request has failed. Usage: `onerror: function() {}` |
| | | onload | Function called when the request has successfully completed. Usage: `onload: function() {}` |
| | | ontimeout | Function called when the author specified timeout has passed before the request completed. Usage: `ontimeout: function() {}` |
| | | onloadend | Function called when the request has completed (either in success or failure). Usage: `onloadend: function() {}` |
| [XMLHttpRequest](https://xhr.spec.whatwg.org/#interface-xmlhttprequest) | attribute | onreadystatechange | The readyState attribute changes value, except when it changes to UNSENT. Usage: `onreadystatechange: function() {}` |
| | | readyState* | Returns the current state, which is one of the readyState code shown below. |
| | | timeout | Terminates fetching after the given time (in milliseconds) has passed. If the fetching has not completed after the time passed and the synchronous flag is unset, a timeout event will be dispatched. |
| | | status | Returns 0 if the state is UNSENT or OPENED, or error flag is set. Otherwise returns the HTTP status code. |
| | | responseType | Sets or returns the response type, which is either `""`, `"blob"`, `"json"`, or `"text"`. |
| | | response | Returns the response entity body, which is either `string`, `Blob` object, `object`, or `string` when responseType is `""`, `"blob"`, `"json"`, or `"text"`, respectively. |
| | | responseText | Returns an empty string if the state is not LOADING or DONE, or error flag is set. Returns the text response entity body when responseType is either `""` or `"text"`. The allowed character set for response text is UTF-8. Otherwise returns an invalidStateError exception with either "Permission denied", "Position unavailable", or "Timeout expired". |
| | method | open(method, url [, async = true [, username = null [, password = null]]]) | Sets the request method, request URL, and synchronous flag.<br>Supported request method : GET, POST  |
| | | send(data = null) | Initiates the request. The optional 'data' argument allows only UTF-8 encoded string type. The argument is ignored if request method is GET. |
| | | abort() | Cancels any network activity. |

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
| [Blob](https://w3c.github.io/FileAPI/#blob) | attribute | size | Returns the size of the byte sequence in number of bytes. |
| | | type | Returns a parsable MIME type. |
| | | isClosed | Returns a boolean value that indicates whether the Blob is in the CLOSED readability state. |
| | method  | slice(start = 0, end = size, contentType = "") | The slice() method returns a new Blob object with bytes ranging from the optional start parameter up to but not including the optional end parameter, and with a type attribute that is the value of the optional contentType parameter. |
| | | close() | The close() method closes a Blob. |


### Page Visibility
Extensions to the Document Object: The document object is extended by the following attributes.

| Interface            | Type   | Name                      | Description |
|----------------------|--------|---------------------------|-------------|
| [Document](https://www.w3.org/TR/page-visibility/#sec-document-interface) | attribute | hidden | Returns true if the Document contained by the top level browsing context (root window in the browser's viewport) is not visible at all. |
| | | visibilityState | Returns one of the following strings: "hidden", or "visible" |
| [VisibilityChange Event](https://www.w3.org/TR/page-visibility/#sec-visibilitychange-event) | Event Handler | visibilitychange | Fire when the content of a tab has become visible or has been hidden. |


### Timing Control for Script-based Animations
Extensions to the Window Object: The window object is extended by the following methods.

| Interface            | Type   | Name                      | Description |
|----------------------|--------|---------------------------|-------------|
| [Window](https://www.w3.org/TR/animation-timing/#Window-interface-extensions) | method | requestAnimationFrame(callback) | Used to signal to the user agent that a script-based animation needs to be resampled. |
| | | cancelAnimationFrame(handle) | Used to cancel a previously made request to schedule an animation frame update. |


### Geolocation
Extensions to the Navigator Object: The navigator is extended by the following attributes and methods.

| Interface            | Type   | Name                      | Description |
|----------------------|--------|---------------------------|-------------|
| [Navigator](https://dev.w3.org/geo/api/spec-source.html#api_description) | attribute | geolocation | `navigator.geolocation` object is used to determine the location information associated with the hosting device. |
| [Geolocation](https://dev.w3.org/geo/api/spec-source.html#api_description) | method | getCurrentPosition(successCallback, errorCallback, options) | Parameters are in following formats:<br>`successCallback`: `function(position) {}`<br>`errorCallback`: `function (positionError) {}`<br>`options`: `PositionOptions` |
| [PositionOptions](https://dev.w3.org/geo/api/spec-source.html#position-options) | attribute | enableHighAccuracy | When enabled, use GPS only to improve the location accuracy. When disabled, use both GPS and WPS. Enabling it may result in slower response times or increased power consumption. Default: false |
| | | timeout | The maximum length of time (expressed in milliseconds) that is allowed to pass from the call to getCurrentPosition() until the corresponding successCallback is invoked. The maximum allowed value is 120. Default: 120 |
| | | maximumAge | Sets to return a cached position whose age is no greater than the specified time in milliseconds. If maximumAge is set to 0, a new position object is acquired immediately. Default: 0 |
| [Position](https://dev.w3.org/geo/api/spec-source.html#position_interface) | attribute | coords | The coords attribute contains a set of geographic coordinates together with their associated accuracy, as well as a set of other optional attributes such as altitude and speed. |
| | | timestamp | The timestamp attribute represents the time when the Position object was acquired in milliseconds. |
| [Coordinates](https://dev.w3.org/geo/api/spec-source.html#coordinates_interface) | attribute | latitude | The latitude attribute is a geographic coordinate specified in decimal degrees. |
| | | longitude | The longitude attribute is a geographic coordinate specified in decimal degrees. |
| | | altitude | The altitude attribute denotes the height of the position, specified in meters above the WGS84 ellipsoid. |
| | | accuracy | The accuracy attribute denotes the accuracy level of the latitude and longitude coordinates. It is specified in meters, and is a non-negative real number. |
| | | altitudeAccuracy | Not supported by the widget engine. Always returns null. |
| | | heading | The heading attribute denotes the direction of travel of the hosting device and is specified in degrees, where 0° ≤ heading < 360°, counting clockwise relative to the true north. |
| | | speed | The speed attribute denotes the magnitude of the horizontal component of the hosting device's current velocity and is specified in meters per second. The value of the speed attribute is a non-negative real number. |
| [PositionError](https://dev.w3.org/geo/api/spec-source.html#position_error_interface) | attribute | code* | Returns the appropriate position error code |
| | | message | Returns an error message describing the details of the error encountered. |

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
