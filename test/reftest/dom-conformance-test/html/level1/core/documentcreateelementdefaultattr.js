
/*
Copyright Â© 2001-2004 World Wide Web Consortium, 
(Massachusetts Institute of Technology, European Research Consortium 
for Informatics and Mathematics, Keio University). All 
Rights Reserved. This work is distributed under the W3CÂ® Software License [1] in the 
hope that it will be useful, but WITHOUT ANY WARRANTY; without even 
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 

[1] http://www.w3.org/Consortium/Legal/2002/copyright-software-20021231
*/



   /**
    *  Gets URI that identifies the test.
    *  @return uri identifier of test
    */
function getTargetURI() {
      return "http://www.w3.org/2001/DOM-Test-Suite/level1/core/documentcreateelementdefaultattr";
   }

var docsLoaded = -1000000;
var builder = null;

//
//   This function is called by the testing framework before
//      running the test suite.
//
//   If there are no configuration exceptions, asynchronous
//        document loading is started.  Otherwise, the status
//        is set to complete and the exception is immediately
//        raised when entering the body of the test.
//
function setUpPage() {
   setUpPageStatus = 'running';
   try {
     //
     //   creates test document builder, may throw exception
     //
     builder = createConfiguredBuilder();
       setImplementationAttribute("validating", true);

      docsLoaded = 0;
      
      var docRef = null;
      if (typeof(this.doc) != 'undefined') {
        docRef = this.doc;
      }
      docsLoaded += preload(docRef, "doc", "hc_staff");
        
       if (docsLoaded == 1) {
          setUpPageStatus = 'complete';
       }
    } catch(ex) {
    	catchInitializationError(builder, ex);
        setUpPageStatus = 'complete';
    }
}



//
//   This method is called on the completion of 
//      each asychronous load started in setUpTests.
//
//   When every synchronous loaded document has completed,
//      the page status is changed which allows the
//      body of the test to be executed.
function loadComplete() {
    if (++docsLoaded == 1) {
        setUpPageStatus = 'complete';
    }
}


/**
* 
    The "createElement(tagName)" method creates an Element 
   of the type specified.  In addition, if there are known attributes
   with default values, Attr nodes representing them are automatically
   created and attached to the element.
   Retrieve the entire DOM document and invoke its 
   "createElement(tagName)" method with tagName="address".
   The method should create an instance of an Element node
   whose tagName is "address".  The tagName "address" has an 
   attribute with default values, therefore the newly created element
   will have them.  

* @author NIST
* @author Mary Brady
* @see http://www.w3.org/TR/1998/REC-DOM-Level-1-19981001/level-one-core#ID-2141741547
* @see http://lists.w3.org/Archives/Public/www-dom-ts/2002Mar/0002.html
*/
function documentcreateelementdefaultattr() {
   var success;
    if(checkInitialization(builder, "documentcreateelementdefaultattr") != null) return;
    var doc;
      var newElement;
      var defaultAttr;
      var child;
      var name;
      var value;
      
      var docRef = null;
      if (typeof(this.doc) != 'undefined') {
        docRef = this.doc;
      }
      doc = load(docRef, "doc", "hc_staff");
      newElement = doc.createElement("address");
      defaultAttr = newElement.attributes;

      child = defaultAttr.item(0);
      assertNotNull("defaultAttrNotNull",child);
name = child.nodeName;

      assertEquals("attrName","street",name);
       value = child.nodeValue;

      assertEquals("attrValue","Yes",value);
       assertSize("attrCount",1,defaultAttr);

}




function runTest() {
   documentcreateelementdefaultattr();
}
