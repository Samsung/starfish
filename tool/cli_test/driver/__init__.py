"""Driver for the CLI integration test.

support   spawns the CLI binary against a local HTTP server and gives each
          test case a CLITestCase base class to subclass.

Everything that depends on how the product is built stays here: the binary
names the build derives from TARGETNAME, the session socket path, and the
fake engine under fixtures/. CLITestCase is the API the test cases use.

The test cases live in the test submodule at test/cli, together with the page
and expected output they load from test/cli/fixtures. They describe behavior
only, so they carry no build or product detail. Adding a test means adding
one test_*.py there: discovery picks it up with no list to update.
"""

# Names this suite's directories under .tmp. Defined here because the runner
# needs it before it can import support, which reads the binary path from the
# environment at import time. Ownership is not per suite: see OWNER_VARIABLE
# in common/storage.py.
STORAGE_GROUP = "test-cli"
