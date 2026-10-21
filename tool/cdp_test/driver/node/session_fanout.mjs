// FIXME: Temporary workaround so accessibility-getRootNode.js can test
// Accessibility.getRootAXNode. Remove this file when Starfish supports
// multiple sessions.
//
// Multiple sessions: a CDP client can attach to the same target more than
// once over one connection. Each attach gets its own session id and its own
// state, such as the enabled domains.
//
// Starfish has one session per target. A second Target.attachToTarget returns
// the same session id. Page.navigate() in the Chromium harness attaches a
// second time. The harness then sends that id's messages only to the new
// Session, and the test, which listens on the first Session, times out.
//
// This module sends each message for a session id to every Session that uses
// the id.

export function installSessionFanout(sessionMap) {
  const groups = new Map();
  const registerDispatcher = sessionMap.set.bind(sessionMap);
  const removeDispatcher = sessionMap.delete.bind(sessionMap);

  sessionMap.set = (sessionId, session) => {
    let group = groups.get(sessionId);
    if (!group) {
      group = new Set();
      groups.set(sessionId, group);
      registerDispatcher(sessionId, {
        _dispatchMessage(message) {
          for (const recipient of group) {
            recipient._dispatchMessage(message);
          }
        },
      });
    }

    if (group.has(session)) {
      return sessionMap;
    }
    group.add(session);

    const disconnect = session.disconnect.bind(session);
    session.disconnect = async (...args) => {
      const result = await disconnect(...args);
      group.delete(session);
      if (!group.size) {
        groups.delete(sessionId);
        removeDispatcher(sessionId);
      }
      return result;
    };
    return sessionMap;
  };
}
