
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
      return "http://www.w3.org/2001/DOM-Test-Suite/level2/core/nodegetlocalname03";
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
       setImplementationAttribute("namespaceAware", true);

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
	The method getLocalName returns the local part of the qualified name of this node.
	
	Ceate two new element nodes and atribute nodes, with and without namespace prefixes.
	Retreive the local part of their qualified names using getLocalName and verrify
	if it is correct.

* @author IBM
* @author Neil Delima
* @see http://www.w3.org/TR/DOM-Level-2-Core/core#ID-NodeNSLocalN
*/
function nodegetlocalname03() {
   var success;
    if(checkInitialization(builder, "nodegetlocalname03") != null) return;
    var doc;
      var element;
      var qelement;
      var attr;
      var qattr;
      var localElemName;
      var localQElemName;
      var localAttrName;
      var localQAttrName;
      
      var docRef = null;
      if (typeof(this.doc) != 'undefined') {
        docRef = this.doc;
      }
      doc = load(docRef, "doc", "hc_staff");
      element = doc.createElementNS("http://www.w3.org/DOM/Test/elem","elem");
      qelement = doc.createElementNS("http://www.w3.org/DOM/Test/elem","qual:qelem");
      attr = doc.createAttributeNS("http://www.w3.org/DOM/Test/attr","attr");
      qattr = doc.createAttributeNS("http://www.w3.org/DOM/Test/attr","qual:qattr");
      localElemName = element.localName;

      localQElemName = qelement.localName;

      localAttrName = attr.localName;

      localQAttrName = qattr.localName;

      assertEquals("nodegetlocalname03_localElemName","elem",localElemName);
       assertEquals("nodegetlocalname03_localQElemName","qelem",localQElemName);
       assertEquals("nodegetlocalname03_localAttrName","attr",localAttrName);
       assertEquals("nodegetlocalname03_localQAttrName","qattr",localQAttrName);
       
}




function runTest() {
   nodegetlocalname03();
}
