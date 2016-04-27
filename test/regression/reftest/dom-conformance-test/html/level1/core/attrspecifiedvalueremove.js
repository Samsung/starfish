
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
      return "http://www.w3.org/2001/DOM-Test-Suite/level1/core/attrspecifiedvalueremove";
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
To respecify the attribute to its default value from
the DTD, the attribute must be deleted.  This will then
make a new attribute available with the "getSpecified()"
method value set to false.
Retrieve the attribute named "street" from the last
child of of the THIRD employee and delete it.  This
should then create a new attribute with its default
value and also cause the "getSpecified()" method to
return false.
This test uses the "removeAttribute(name)" method
from the Element interface and the "getNamedItem(name)"
method from the NamedNodeMap interface.

* @author NIST
* @author Mary Brady
* @see http://www.w3.org/TR/1998/REC-DOM-Level-1-19981001/level-one-core#ID-6D6AC0F9
* @see http://lists.w3.org/Archives/Public/www-dom-ts/2002Mar/0002.html
*/
function attrspecifiedvalueremove() {
   var success;
    if(checkInitialization(builder, "attrspecifiedvalueremove") != null) return;
    var doc;
      var addressList;
      var testNode;
      var attributes;
      var streetAttr;
      var state;
      
      var docRef = null;
      if (typeof(this.doc) != 'undefined') {
        docRef = this.doc;
      }
      doc = load(docRef, "doc", "hc_staff");
      addressList = doc.getElementsByTagName("address");
      testNode = addressList.item(2);
      testNode.removeAttribute("street");
      attributes = testNode.attributes;

      streetAttr = attributes.getNamedItem("street");
      assertNotNull("streetAttrNotNull",streetAttr);
state = streetAttr.specified;

      assertFalse("attrSpecifiedValueRemoveAssert",state);

}




function runTest() {
   attrspecifiedvalueremove();
}
