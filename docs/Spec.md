# Specification

## HTML

| HTML Tag | Attribute | Allowed Value | Usage | Note |
|----------|-----------|---------------|-------|------|
| Global Attribute | class | string | \<element class="classname"\> | \<css_styles\> must conform to the widget engine's CSS rules |
|                  | dir   | ltr &#124; rtl | \<element class="ltr"\> | |


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
| Margin | margin | \<margin-width\>{1,4} | The margin shorthand property sets all the margin properties in one declaration | The margin properties specify the width of the margin area of a box. \<margin-width\> may take one of the following values: \<length\>, \<percentage\>, auto. (Also check [Length](https://www.w3.org/TR/CSS2/syndata.html#length-units)) |
| | margin-bottom | \<margin-width\> &#124; \<percentage\> &#124; auto | Sets the bottom margin of an element | |

## Selectors

| Selectors | Type | Pattern | Usage | Description |
|-----------|------|---------|-------|-------------|
| [Selectors](https://www.w3.org/TR/CSS2/selector.html) | Universal Selector | * | * | Selects all elements |
| | Type Selector | element | p | Selects all \<p\> elements. The 'OR' condition is allowed (e.g., element, element) |

## Additional Supported APIs

### XMLHttpRequest
| Interface            | Type   | Name                      | Description |
|----------------------|--------|---------------------------|-------------|
| [XMLHttpRequestEventTarget](https://xhr.spec.whatwg.org/#xmlhttprequesteventtarget) | attribute | onloadstart | Function called when the request starts. Usage: ``onloadstart: function() {}`` |
| | | onprogress | Function called when transmitting data. Usage: ``onprogress: function() {}`` |

| readyStateCode | Description | Numeric Value |
|----------------|-------------|---------------|
| UNSENT         | The object has been constructed. | 0 |


### Blob
| Interface            | Type   | Name                      | Description |
|----------------------|--------|---------------------------|-------------|
| [Blob](https://w3c.github.io/FileAPI/#blob) | attribute | size | Returns the size of the byte sequence in number of bytes. |
| | | type | Returns a parsable MIME type. |


### Page Visibility
| Interface            | Type   | Name                      | Description |
|----------------------|--------|---------------------------|-------------|
| [Document](https://www.w3.org/TR/page-visibility/#sec-document-interface) | attribute | hidden | Returns true if the Document contained by the top level browsing context (root window in the browser's viewport) is not visible at all. |
| | | visibilityState | Returns one of the following strings: "hidden", or "visible" |


### Timing Control for Script-based Animations
| Interface            | Type   | Name                      | Description |
|----------------------|--------|---------------------------|-------------|
| [Window](https://www.w3.org/TR/animation-timing/#Window-interface-extensions) | method | requestAnimationFrame(callback) | Used to signal to the user agent that a script-based animation needs to be resampled. |
| | | cancelAnimationFrame(handle) | Used to cancel a previously made request to schedule an animation frame update. |


### Geolocation
| Interface            | Type   | Name                      | Description |
|----------------------|--------|---------------------------|-------------|
| [Navigator](https://dev.w3.org/geo/api/spec-source.html#api_description) | attribute | geolocation | ``navigator.geolocation`` object is used to determine the location information associated with the hosting device. |

| Error Code | Description | Numeric Value |
|------------|-------------|---------------|
| PERMISSION\_DENIED | The location acquisition process failed because the widget does not have permission to use the Geolocation API. | 1 |

## Web Device API
| API            | Description | Note |
|----------------|-------------|------|
| [Application](https://developer.tizen.org/dev-guide/2.3.1/org.tizen.web.apireference/html/device_api/wearable/tizen/application.html) | This API allows a widget to launch and access installed applications. Note that launching other Web widgets installed on the same device is deprecated in Tizen 3.0, so such use is not recommended. | Unsupported methods:<br> - ApplicationManager: Application getCurrentApplication()<br> - Application: void exit()<br> - Application: void hide()<br> - Application: RequestedApplicationControl getRequestedAppControl() |


