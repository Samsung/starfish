var panel_obj = document.getElementById("content_panel");
if (panel_obj) {
    var encoded_content = "≤‚ ‘";
    if (encoded_content == panel_obj.innerHTML){
        //panel_obj.innerHTML = "SUCCESS";
        console.log("SUCCESS");
    }
    else{
        //panel_obj.innerHTML = "FAILURE";
        console.log("FAILURE");
    }
    try {
        wptTestEnd();
    } catch(e) {
        console.log(e);
    }
  if (window.testRunner)
    testRunner.notifyDone();
}
