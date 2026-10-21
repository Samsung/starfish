"""Driver for the external CDP contract test.

The layers are kept apart on purpose:

    client         speaks CDP over HTTP discovery and a WebSocket
    launcher       finds a CDP-enabled build and runs one Starfish process
    static_server  serves the test pages over HTTP
    case           executes and asserts one case
    testlist       reads the active and skipped list, computes coverage

Nothing here imports Starfish, so the suite stays a black-box test.
"""
