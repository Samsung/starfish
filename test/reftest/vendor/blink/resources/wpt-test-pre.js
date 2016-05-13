var self = window;
Object.defineProperty(Element.prototype, "innerHTML", { get:function() { return ""}, set:function(d) { console.log(d); }, configurable: true})

function shouldInclude(s, d) {
  var _s = eval(s);
  var ret = _s.indexOf(d);
  if (ret !== -1)
    debug('<span><span class="pass">PASS</span> ' + _s + '</span>');
  else
    debug('<span><span class="pass">FAIL</span> ' + s + ' Should include ' + d + '</span>');
}

function wptTestEnd() {
	try {
    	wptTestEnd();
	} catch(e) {
	    console.log(e);
	}
}