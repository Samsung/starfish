function buildUI(merged_list) {
    function generateClassName(item) {
        result = 'subitem';
        if (item.mdnonly) result += ' type_unimplemented';
        if (item.experimental) result += ' type_experimental';
        if (item.deprecated) result += ' type_deprecated';
        if (item.obsolete) result += ' type_obsolete';
        if (item.not_standardized) result += ' type_not_standardized';
        return result;
    }
    function generateUnimplementedTag() {
        var spanref = document.createElement('span');
        spanref.className = 'subtag subtag_unimplemented';
        spanref.title = 'Starfish does not support';
        spanref.textContent = 'Unimpl';
        return spanref;
    }
    function generateExperimentalTag() {
        var spanref = document.createElement('span');
        spanref.className = 'subtag subtag_experimental';
        spanref.title = 'MDN experimental API';
        spanref.textContent = 'MDN exp';
        return spanref;
    }
    function generateDeprecatedTag() {
        var spanref = document.createElement('span');
        spanref.className = 'subtag subtag_deprecated';
        spanref.title = 'Deprecated API: still work';
        spanref.textContent = 'Depr';
        return spanref;
    }
    function generateObsoleteTag() {
        var spanref = document.createElement('span');
        spanref.className = 'subtag subtag_obsolete';
        spanref.title = 'Obsolete API: no longer guaranteed to work';
        spanref.textContent = 'Obs';
        return spanref;
    }
    function generateNonStandardTag() {
        var spanref = document.createElement('span');
        spanref.className = 'subtag subtag_not_standardized';
        spanref.title = 'Has not been standardized';
        spanref.textContent = 'Non std';
        return spanref;
    }
    function generatePartialTag() {
        var spanref = document.createElement('span');
        spanref.className = 'subtag subtag_partial';
        spanref.title = 'Starfish partial support';
        spanref.textContent = 'Partial';
        return spanref;
    }
    function generateFlagTag(name) {
        var spanref = document.createElement('span');
        spanref.className = 'subtag subtag_flag';
        spanref.textContent = name;
        return spanref;
    }
    var lastSubTitle;
    for (var i = 0; i < merged_list.length; i++) {
        var item = merged_list[i];
        var divref = document.createElement('div');
        divref.className = generateClassName(item);
        var aref = document.createElement('a');
        aref.textContent = item.name;
        if (item.mdnonly) {
            aref.href = 'https://developer.mozilla.org/en-US/docs/Web/API/' + item.name;
            divref.appendChild(aref);
            divref.appendChild(generateUnimplementedTag());
        } else {
            aref.href = './autogen_' + item.name + '.html';
            divref.appendChild(aref);
        }
        if (item.flags && item.flags.length) {
            for (var j = 0; j < item.flags.length; j++) {
                divref.appendChild(generateFlagTag(item.flags[j]));
            }
        }
        if (item.experimental) divref.appendChild(generateExperimentalTag());
        if (item.deprecated) divref.appendChild(generateDeprecatedTag());
        if (item.obsolete) divref.appendChild(generateObsoleteTag());
        if (item.not_standardized) divref.appendChild(generateNonStandardTag());
        if (item.has_unimplemented) divref.appendChild(generatePartialTag());
        if (lastSubTitle != item.name[0]) {
            var subref = document.createElement('div');
            subref.className = 'subtitle';
            subref.textContent = item.name[0];
            lastSubTitle = item.name[0];
            dynamic_root.appendChild(subref);
        }
        dynamic_root.appendChild(divref);
    }
}

function showSummary(total, support, timestamp) {
    function updatePercent(total, support) {
        summary_rate_percent.textContent = Math.floor(support/total*1000)/10 + '%';
    }
    summary_total_count.textContent = total;
    summary_support_count.textContent = support;
    summary_timestamp.textContent = 'MDN sync: ' + timestamp;
    updatePercent(total, support);
    summary.style.display = 'block';
    var disabled = {
        'type_unimplemented': false,
        'type_experimental': false,
        'type_not_standardized': false,
        'type_deprecated': false,
        'type_obsolete': false,
    }
    function changeLabel(checkbox, queryClassName) {
        checkbox.checked = false;
        var items = document.getElementsByClassName(queryClassName);
        checkbox.onchange = function() {
            var label = checkbox.nextElementSibling;
            var totalcount = parseInt(summary_total_count.textContent);
            var supportcount = parseInt(summary_support_count.textContent);
            if (checkbox.checked) {
                disabled[queryClassName] = true;
                for (var i = 0; i < items.length; i++) {
                    if (items[i].style.display != 'none') {
                        items[i].style.display = 'none';
                        totalcount--;
                        if (!items[i].className.includes('type_unimplemented')) {
                            supportcount--;
                        }
                    }
                }
            } else {
                disabled[queryClassName] = false;
                for (var i = 0; i < items.length; i++) {
                    var canRevive = true;
                    for (var key in disabled) {
                        if (disabled.hasOwnProperty(key) &&
                            key != queryClassName &&
                            disabled[key] &&
                            items[i].className.includes(key)) {
                            canRevive = false;
                            break;
                        }
                    }
                    if (canRevive && items[i].style.display == 'none') {
                        items[i].style.display = 'block';
                        totalcount++;
                        if (!items[i].className.includes('type_unimplemented')) {
                            supportcount++;
                        }
                    }
                }
            }
            summary_total_count.textContent =  totalcount;
            summary_support_count.textContent =  supportcount;
            updatePercent(totalcount, supportcount);
        }
    }
    changeLabel(check_unimpl, 'type_unimplemented');
    changeLabel(check_mdn_exp, 'type_experimental');
    changeLabel(check_non_standard, 'type_not_standardized');
    changeLabel(check_deprecated, 'type_deprecated');
    changeLabel(check_obsolte, 'type_obsolete');
}

function runWithMDNData(starfish_data) {
    var request = new XMLHttpRequest();
    request.addEventListener("load", function() {
        var merged_list = [];
        var supportcount = 0;
        var starfish_data_map = starfish_data.reduce(function(map, obj) {
            map[obj.name] = { 'has_unimplemented': obj.has_unimplemented, 'flags': obj.flags };
            return map;
        }, {});
        var el = document.createElement('html');
        el.innerHTML = this.responseText;
        var items = el.getElementsByClassName('indexListRow');
        for (var i = 0; i < items.length; i++) {
            var currentElement = items[i].firstElementChild;
            var interface = {};
            // Get name
            interface.name = currentElement.firstElementChild.firstElementChild.textContent;
            // Get flags
            currentElement = currentElement.nextElementSibling;
            while (currentElement) {
                var type = currentElement.firstElementChild.firstElementChild.className;
                if (type == 'icon-beaker') {
                    interface.experimental = true;
                } else if (type == 'icon-trash') {
                    interface.obsolete = true;
                } else if (type == 'icon-thumbs-down-alt') {
                    interface.deprecated = true;
                } else if (type == 'icon-warning-sign') {
                    interface.not_standardized = true;
                }
                currentElement = currentElement.nextElementSibling;
            }
            // Merge starfish
            if (starfish_data_map.hasOwnProperty(interface.name)) {
                interface.has_unimplemented = starfish_data_map[interface.name].has_unimplemented;
                interface.flags = starfish_data_map[interface.name].flags;
                supportcount++;
            } else {
                interface.mdnonly = true;
            }
            merged_list.push(interface);
        }
        buildUI(merged_list);
        showSummary(items.length, supportcount, new Date());
    });
    request.addEventListener("error", function() {
        console.log("Unable to load MDN webapi list");
        buildUI(starfish_data.sort(function(a, b) {
            return a.name.charCodeAt(0) - b.name.charCodeAt(0);
        }));
    });
    request.open("GET", "https://developer.mozilla.org/en-US/docs/Web/API");
    request.send();
}

function run(starfish_data) {
    buildUI(starfish_data.sort(function(a, b) {
        return a.name.charCodeAt(0) - b.name.charCodeAt(0);
    }));
}
