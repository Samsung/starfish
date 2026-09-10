"""Pieces shared by the test harnesses under tool/.

Only put something here when two harnesses need the same behavior, not when
two harnesses happen to look alike. Today that is storage: both give each
test process a private HOME and both have to clean up after a run that was
interrupted.
"""
