var win = nw.Window.get();

document.body.bgColor = 'white';
var style = document.createElement('style'),
    text = document.createTextNode('* {       '
        + ' font-family: Ahem !important; '
        + ' }');
    //            text2 = document.createTextNode('html { background-color:white; } ');
    var style2 = document.createElement('style');
    style2.textContent = "        ::-webkit-scrollbar {width:0px;height:0px;}        ::-webkit-scrollbar-button {background-color:rgba(156,155,155,1);}        ::-webkit-scrollbar-track {background-color:rgba(208,204,204,1);}        ::-webkit-scrollbar-track-piece {background-color:rgba(100,96,96,1);}        ::-webkit-scrollbar-thumb {height:0px; background-color:rgba(124,122,122,1); border-radius:0px;}        ::-webkit-scrollbar-corner {background-color:rgba(238,238,238,1);}        ";
    style.setAttribute('type', 'text/css');
    style.appendChild(text);
    //        if (window.getComputedStyle(document.body)["background-color"] == "rgba(0, 0, 0, 0)")
    //           style.appendChild(text2);
    document.body.appendChild(style, document.head.firstChild);
    document.body.appendChild(style2, document.head.firstChild);

