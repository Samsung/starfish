var win = nw.Window.get();

document.body.bgColor = 'white';
var style = document.createElement('style'),
    text = document.createTextNode('* { font-family: Ahem !important; }');
//            text2 = document.createTextNode('html { background-color:white; } ');
style.setAttribute('type', 'text/css');
style.appendChild(text);
//        if (window.getComputedStyle(document.body)["background-color"] == "rgba(0, 0, 0, 0)")
//           style.appendChild(text2);
document.body.appendChild(style, document.head.firstChild);

