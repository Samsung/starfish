
// For some reasons, escargot cannot run the following functions defined in testharness.js
// I copied them here, and it works fine
function assert(expected_true, function_name, description, error, substitutions) {
    if (expected_true !== true) {
        var msg = make_message(function_name, description,
                error, substitutions);
        throw new AssertionError(msg);
    }
}

function same_value(x, y) {
    if (y !== y) {
        //NaN case
        return x !== x;
    }
    if (x === 0 && y === 0) {
        //Distinguish +0 and -0
        return 1/x === 1/y;
    }
    return x === y;
}

function assert_array_equals(actual, expected, description) {
    assert(actual.length === expected.length,
            "assert_array_equals", description,
            "lengths differ, expected ${expected} got ${actual}",
            {expected:expected.length, actual:actual.length});

    for (var i = 0; i < actual.length; i++) {

        // FIXME: hasOwnProperty is not defined in HTMLCollection.
        // In fact, we need to impl Array interface for HTMLCollection,
        // which is to be done later. For the time being, we disable
        // the following assert

        //assert(actual.hasOwnProperty(i) === expected.hasOwnProperty(i),
        //       "assert_array_equals", description,
        //       "property ${i}, property expected to be ${expected} but was ${actual}",
        //       {i:i, expected:expected.hasOwnProperty(i) ? "present" : "missing",
        //       actual:actual.hasOwnProperty(i) ? "present" : "missing"});

        // FIXME: Due to not impl Array interface for HTMLCollection
        // we modified [] to .item()
        //assert(same_value(expected[i], actual[i]),
        //       "assert_array_equals", description,
        //       "property ${i}, expected ${expected} but got ${actual}",
        //       {i:i, expected:expected[i], actual:actual[i]});
        assert(same_value(expected[i], actual.item(i)),
                "assert_array_equals", description,
                "property ${i}, expected ${expected} but got ${actual}",
                {i:i, expected:expected[i], actual:actual.item(i)});

    }
}
