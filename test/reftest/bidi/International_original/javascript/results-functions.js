function getStatus (score) {
	if (score=='') { return 'none'; }
	
	x = score.split('/');
	if (x[0]!='. ') { x[0] = true; } else { x[0] = false; }
	if (x[1]!=' . ') { x[1] = true; } else { x[1] = false; }
	if (x[2]!=' .') { x[2] = true; } else { x[2] = false; }
	
	if (!x[0] && !x[1] && !x[2]) { return 'none'; }
	if (x[0] && !x[1] && !x[2]) { return 'pass'; }
	if (!x[0] && x[1] && !x[2]) { return 'fail'; }
	if (!x[0] && !x[1] && x[2]) { return 'uncertain'; }
	if (x[0] && !x[1] && x[2]) { return 'pass uncertain'; }
	if (x[0] && x[1]) { return 'pass fail'; }
	if (!x[0] && x[1] && x[2]) { return 'fail uncertain'; }
	
	else { return 'n/a'; }
	}



function makeCaseMap (results) {
    var res = {};
    for (var i = 0, n = results.length; i < n; i++) {
        res[results[i].testcase] = results[i];
    }
    return res;
}


function generateStuffInPage (error, data) { 	
    if (error) return alert("There was an error, sorry.");
	//console.log(error, data);
	
	var resultsMap = makeCaseMap(data.results);

//alert(resultsMap.toSource())

	for (i=0; i<testlist.length; i++) {
		var testcase = resultsMap[testlist[i]].testcase;
		var engineptr = resultsMap[testlist[i]].byEngine; 
		
		
		if (engineptr.gecko) {
		score = ((engineptr.gecko.pass > 0) ? engineptr.gecko.pass : ".") + ' / ' + ((engineptr.gecko.fail > 0) ? engineptr.gecko.fail : ".") + ' / ' + ((engineptr.gecko.uncertain > 0) ? engineptr.gecko.uncertain : "."); 
		//score = engineptr.gecko.pass + ' / ' + engineptr.gecko.fail + ' / ' + engineptr.gecko.uncertain;
		document.getElementById(testcase+'-gecko').innerHTML = score;
		document.getElementById(testcase+'-gecko').className = getStatus(score);
		}
		if (engineptr.presto) {
		score = ((engineptr.presto.pass > 0) ? engineptr.presto.pass : ".") + ' / ' + ((engineptr.presto.fail > 0) ? engineptr.presto.fail : ".") + ' / ' + ((engineptr.presto.uncertain > 0) ? engineptr.presto.uncertain : ".");
		document.getElementById(testcase+'-presto').innerHTML = score;
		document.getElementById(testcase+'-presto').className = getStatus(score);
		}
		if (engineptr.trident != null) {
		score = ((engineptr.trident.pass > 0) ? engineptr.trident.pass : ".") + ' / ' + ((engineptr.trident.fail > 0) ? engineptr.trident.fail : ".") + ' / ' + ((engineptr.trident.uncertain > 0) ? engineptr.trident.uncertain : ".");
		document.getElementById(testcase+'-trident').innerHTML = score;
		document.getElementById(testcase+'-trident').className = getStatus(score);
		}
		if (engineptr.webkit) {
		score = ((engineptr.webkit.pass > 0) ? engineptr.webkit.pass : ".") + ' / ' + ((engineptr.webkit.fail > 0) ? engineptr.webkit.fail : ".") + ' / ' + ((engineptr.webkit.uncertain > 0) ? engineptr.webkit.uncertain : ".");
		document.getElementById(testcase+'-webkit').innerHTML = score;
		document.getElementById(testcase+'-webkit').className = getStatus(score);
		}
		}
	
	}
