(function ($) {
    "use strict";
    
    window.TestFramework = {
        base:   "http://w3c-test.org/framework/api/"
    ,   _get:   function (uri, cb) {
            uri = this.base + uri;
            // console.log(uri, cb);
            if (!cb) return;
            $.ajax({
                url:        uri
            ,   dataType:   "jsonp"
            ,   success:    function (data) {
                    // console.log("RECEIVED", data);
                    if (!data) return cb({ errors: ["No data"] });
                    data.errors ? cb(data, null) : cb(null, data);
                }
            });
        }
    ,   _post:   function (uri, data, cb) {
            uri = this.base + uri;
            // console.log(uri, data);
            if (!cb) return;
            $.ajax({
                url:        uri
            ,   method:     "POST"
            ,   dataType:   "jsonp"
            ,   data:       data
            ,   success:    function (data) {
                    // console.log("RECEIVED", data);
                    if (!data) return cb({ errors: ["No data"] });
                    data.errors ? cb(data, null) : cb(null, data);
                }
            });
        }
    ,   _put:   function (uri, id, data, cb) {
            uri = uri + "/" + id;
            data += "&_method=PUT";
            this._post(uri, data, cb);
        }
    ,   _delete:   function (uri, id, cb) {
            uri = id ? uri + "/" + id : uri;
            this._post(uri, "_method=DELETE", cb);
        }
    ,   _item:          function (coll, id, cb) { this._get(coll + "/" + id, cb); }
    // ancilliary data
    ,   formats:        function (cb) { this._get("formats", cb); }
    ,   flags:          function (cb) { this._get("flags", cb); }
    // user information
    ,   user:           function (cb) { this._get("user", cb); }
    ,   login:          function (data, cb) { this._post("login", data, cb); }
    ,   logout:         function (cb) { this._post("logout", {}, cb); }
    // specs
    ,   specifications: function (cb) { this._get("specifications", cb); }
    ,   specification:  function (id, cb) { this._item("specifications", id, cb); }
    ,   saveSpecification:  function (id, data, cb) { this._put("specifications", id, data, cb); }
    ,   addSpecification:   function (data, cb) { this._post("specifications", data, cb); }
    ,   localSections:  function (id, cb) { this._item("local-sections", id, cb); }
    ,   remoteSections: function (id, cb) { this._item("remote-sections", id, cb); }
    ,   importSections: function (id, cb) { this._post("import-sections/" + id, {}, cb); }
    // TS
    ,   testsuites:     function (cb) { this._get("testsuites", cb); }
    ,   testsuite:      function (id, cb) { this._item("testsuites", id, cb); }
    ,   saveTestsuite:  function (id, data, cb) { this._put("testsuites", id, data, cb); }
    ,   addTestsuite:   function (data, cb) { this._post("testsuites", data, cb); }
    ,   importTestCases:    function (id, data, cb) { this._post("import-test-cases/" + id, data, cb); }
    ,   testCases:      function (id, cb) { this._item("test-cases", id, cb); }
    ,   findTestCases:  function (id, data, cb) { this._post("find-test-cases/" + id, data, cb); }
    ,   sectionsForSuite:   function (id, cb) { this._item("sections-for-suite", id, cb); }
    ,   submitResult:   function (id, data, cb) { this._post("submit-result/" + id, data, cb); }
    ,   results:        function (id, data, cb) { this._get("result/" + id + "?" + data, cb); }
    ,   resultSummary:        function (id, data, cb) { this._get("result-summary/" + id + "?" + data, cb); }
    // UA
    ,   currentUA:  function (cb) { this._get("user-agent", cb); }
    ,   setUA:      function (data, cb) { this._post("user-agent", data, cb); }
    ,   resetUA:    function (cb) { this._delete("user-agent", null, cb); }
    ,   knownUAs:   function (cb) { this._get("known-user-agents", cb); }
    };
})(jQuery);
