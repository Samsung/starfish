
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
      return "http://www.w3.org/2001/DOM-Test-Suite/level2/core/documenttypepublicid01";
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
    The method getInternalSubset() returns the public identifier of the external subset.
  
    Create a new DocumentType node with the value "PUB" for its publicId.
    Check the value of the publicId attribute using getPublicId().

* @author IBM
* @author Neil Delima
* @see http://www.w3.org/TR/DOM-Level-2-Core/core#ID-Core-DocType-publicId
* @see http://www.w3.org/Bugs/Public/show_bug.cgi?id=259
*/
function documenttypepublicid01() {
   var success;
    if(checkInitialization(builder, "documenttypepublicid01") != null) return;
    var doc;
      var docType;
      var domImpl;
      var publicId;
      var nullNS = null;

      
      var docRef = null;
      if (typeof(this.doc) != 'undefined') {
        docRef = this.doc;
      }
      doc = load(docRef, "doc", "hc_staff");
      //domImpl = doc.implementation;
//docType = domImpl.createDocumentType("l2:root","PUB",nullNS);
      publicId = doc.doctype.publicId;

      assertEquals("documenttypepublicid01","-//W3C//DTD HTML 4.01//EN",publicId);
      
}




function runTest() {
   documenttypepublicid01();
}
