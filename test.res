
[1mtest-async[22m
✔ AsyncConnection
✔ AsyncQueryWithError
✔ AsyncFetch

[1mtest-binding[22m
✔ ConnectionBinding
✔ FBResultBinding

[1mtest-events[22m
✔ noEvent
[31m✖ oneEvent[39m

Error: Expected 3 assertions, 1 ran
    at Object.done (/root/node_firebird/tools/nodeunit/lib/types.js:119:25)
    at Object._onTimeout (/root/node_firebird/tests/def/test-events.js:94:13)
    at Timer.callback (timers.js:95:39)

[31m✖ oneEventBetween[39m

Error: Expected 3 assertions, 1 ran
    at Object.done (/root/node_firebird/tools/nodeunit/lib/types.js:119:25)
    at Object._onTimeout (/root/node_firebird/tests/def/test-events.js:120:13)
    at Timer.callback (timers.js:95:39)

Gen Event39
Gen Event38
Gen Event37
Gen Event36
Gen Event35
Gen Event34
Gen Event33
Gen Event32
Gen Event31
Gen Event30
Gen Event29
Gen Event28
Gen Event27
Gen Event26
Gen Event25
Gen Event24
Gen Event23
Gen Event22
Gen Event21
Gen Event20
Gen Event19
Gen Event18
Gen Event17
Gen Event16
Gen Event15
Gen Event14
Gen Event13
Gen Event12
Gen Event11
Gen Event10
Gen Event9
Gen Event8
Gen Event7
Gen Event6
Gen Event5
Gen Event4
Gen Event3
Gen Event2
Gen Event1
Gen Event0
[31m✖ TensOfEvents[39m

Assertion Message: [35mWe have 40 events[39m
AssertionError: false == true
    at Object.ok (/root/node_firebird/tools/nodeunit/lib/types.js:81:39)
    at Object._onTimeout (/root/node_firebird/tests/def/test-events.js:153:10)
    at Timer.callback (timers.js:95:39)


[1mtest-fbres[22m
✔ FBResult
✔ FBResultFetchSyncArgs
✔ FBResFetchSyncOne
✔ FBResFetchMany

[1mtest-sync[22m
✔ SyncConnection
✔ SyncDDLTransaction
✔ SyncTransCommit
✔ SyncTransRollback

[1m[31mFAILURES: [39m[22m3/61 assertions failed (24414ms)
event_callback 144507152 first_time=1 count=1
event_callback 144512840 first_time=1 count=1
event_callback 144525056 first_time=1 count=1
event_callback 144525056 first_time=1 count=2
event_callback 144533168 first_time=1 count=1
event_callback 144533168 first_time=1 count=2
event_callback 144533168 first_time=1 count=3
event_callback 144533168 first_time=1 count=4
event_callback 144533168 first_time=1 count=5
event_callback 144533168 first_time=1 count=6
event_callback 144533168 first_time=1 count=7
event_callback 144533168 first_time=1 count=8
event_callback 144533168 first_time=1 count=9
event_callback 144533168 first_time=1 count=10
event_callback 144533168 first_time=1 count=11
event_callback 144533168 first_time=1 count=12
event_callback 144533168 first_time=1 count=13
event_callback 144533168 first_time=1 count=14
event_callback 144534464 first_time=1 count=1
event_callback 144534464 first_time=1 count=2
event_callback 144534464 first_time=1 count=3
event_callback 144534464 first_time=1 count=4
event_callback 144534464 first_time=1 count=5
event_callback 144534464 first_time=1 count=6
event_callback 144534464 first_time=1 count=7
event_callback 144534464 first_time=1 count=8
event_callback 144534464 first_time=1 count=9
event_callback 144534464 first_time=1 count=10
event_callback 144534464 first_time=1 count=11
event_callback 144534464 first_time=1 count=12
event_callback 144534464 first_time=1 count=13
event_callback 144534464 first_time=1 count=14
event_callback 144535760 first_time=1 count=1
event_callback 144535760 first_time=1 count=2
event_callback 144535760 first_time=1 count=3
event_callback 144535760 first_time=1 count=4
event_callback 144535760 first_time=1 count=5
event_callback 144535760 first_time=1 count=6
event_callback 144535760 first_time=1 count=7
event_callback 144535760 first_time=1 count=8
event_callback 144535760 first_time=1 count=9
event_callback 144533168 first_time=1 count=15
event_callback 144534464 first_time=1 count=15
event_callback 144535760 first_time=1 count=10
