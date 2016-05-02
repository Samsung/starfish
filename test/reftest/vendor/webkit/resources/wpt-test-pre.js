var self = window;
Object.defineProperty(Element.prototype, "innerHTML", { get:function() { return ""}, set:function(d) { console.log(d); }, configurable: true})
