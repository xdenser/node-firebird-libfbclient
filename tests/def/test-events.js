/*
Copyright by Denys Khanzhiyev
See license text in LICENSE file
*/

// Load configuration
var cfg = require("../config").cfg;

// Require modules
var
  fb_binding = require("../../firebird.js");
  

var zexports = {};  
 

function Init()
{
  var conn = fb_binding.createConnection();
  conn.connectSync(cfg.db, cfg.user, cfg.password, cfg.role);
  conn.querySync("\
                 create or alter procedure DoEvent(\
                  ev_name VARCHAR(30))\
                  as\
                  begin\
                   post_event :ev_name;\
                  end");
  conn.disconnect();
}

function Connect(){
  var conn = fb_binding.createConnection();
  conn.connectSync(cfg.db, cfg.user, cfg.password, cfg.role);
  return conn;
}

function GenEvent(event){
 var con = Connect();
 con.querySync("execute procedure DoEvent('"+event+"')");
 con.commitSync();
 con.disconnect();
// console.log("GenEvent "+event); 
}

function WaitForFinish(finished,clean,timeout){
  var timedout = false;
  var tid = setTimeout(function(){
    timedout = true;
  },timeout);
  
  
  /*process.nextTick(function loop(){
     if(finished.call()||timedout){
       clearTimeout(tid);
       clean.call();
     }
     else  process.nextTick(loop);
  });
  */
  setTimeout(function loop(){
     if(finished.call()||timedout){
       clearTimeout(tid);
       clean.call();
     }
     else setTimeout(loop,1);
  },1);
       
}

function CleanUp(){
  var con = fb_binding.createConnection();
  con.connectSync(cfg.db, cfg.user, cfg.password, cfg.role);
  con.querySync("drop procedure DoEvent;");
  con.disconnect();
}


exports.noEvent = function(test) {
  test.expect(2);
  Init();
  var conn = Connect();
  test.ok(conn.connected,"Not connected to database");
  var eName = "Event1";
  var regevents = [];
  conn.on("fbevent",function(event,count){
      //    console.log("this should not be called in that test "+event+' '+count ); 
      if (count > 0) {
          regevents.push({ name: event, count: count });
      }
  });  
  conn.addFBevent(eName);
//  console.log('after add Event1');
//  conn.addFBevent('another');
//  console.log('after add another');
// Wait 1 sec for event
  setTimeout(function () {
       conn.disconnect();
       CleanUp();
       test.equal(regevents.length, 0, "incorrect number of callback calls " + JSON.stringify(regevents));
       test.done();    
  }, 1000);
  
}

exports.oneEvent = function(test) {
  test.expect(4);
  Init();
  
  var conn = Connect();
  test.ok(conn.connected,"Not connected to database");
  var eName = "Event1";
  var finished = false;
  conn.addFBevent(eName);
  var regevents = [];
  conn.on("fbevent", function (event, count) {
      if (count > 0) {
          regevents.push({ name: event, count: count });
          finished = true;
      }
  });  

  GenEvent(eName);
  
  // Wait 5 sec for event
  WaitForFinish(function(){ return finished; },
  function(){
       conn.disconnect();
       CleanUp();
       test.equal(regevents.length, 1, "incorrect number of callback calls" + JSON.stringify(regevents));
       test.equal(regevents[0].name, eName, "incorrect event name");
       test.equal(regevents[0].count, 1, "wrong number of events");
       test.done();    
  }, 5000);
}

exports.oneEventBetween = function(test) {
  test.expect(4);
  Init();
  
  var conn = Connect();
  test.ok(conn.connected,"Connected to database");
  var eName = "Event1";
  var finished = false;
  var regevents = [];
  conn.on("fbevent", function (event, count) {
      console.log("event", event, count);
      if (count > 0) {
          regevents.push({ name: event, count: count });
          finished = true;
      }
  });  

  conn.addFBevent(eName);
  
  GenEvent(eName);
    
  conn.addFBevent("strange");  
   
  // Wait 5 sec for event
  WaitForFinish(function(){ return finished; },
  function(){
       conn.disconnect();
       CleanUp();
       test.equal(regevents.length, 1, "incorrect number of callback calls" + JSON.stringify(regevents));
       test.equal(regevents[0].name, eName, "incorrect event name");
       test.equal(regevents[0].count, 1, "wrong number of events");
       test.done();    
  }, 5000);
}


exports.TensOfEvents = function(test){
  test.expect(2);
  Init();
  var conn = Connect();
  test.ok(conn.connected,"Connected to database");
  
  var evcount = 0;
  var expected_count = 100; //400;?
  conn.on("fbevent", function(event,count){
    //console.log("->"+event+" "+count);
    evcount++;
  });
  
  var events = [];
  var en;
  for(var i = 0; i<expected_count; i++){
    en = 'Event'+i;
    events.push(en);
    conn.addFBevent(en);    
  }
  events = events.reverse();
  for(var i = 0; i<expected_count; i++){
    setTimeout((function(idx){
     return  function(){
     GenEvent(events[idx]);
    // console.log("Gen "+events[idx]);
    };})(i),0);
  }

  WaitForFinish(function(){ return (evcount == expected_count); },
  function(){
       test.ok(evcount == expected_count, "We have "+ evcount + " events. Expected: " + expected_count);
       conn.disconnect();
       CleanUp();
       test.done();    
  }, expected_count*500);
}

exports.AddAndDelete = function(test){

  test.expect(2);
  Init();
  var conn = Connect();
  test.ok(conn.connected,"Connected to database");
  var called = false;
  conn.on("fbevent", function(event,count){
    //console.log("->"+event+" "+count);
    called = true;
  });
  var eN = 'eventName';
  conn.addFBevent(eN);
  conn.deleteFBevent(eN);
  GenEvent(eN);
  
  setTimeout(function(){
       test.ok(!called,"Event not called");
       conn.disconnect();
       CleanUp();
       test.done();    
  }, 1000);
  
}

exports.AddAndDeleteMultiSimple = function(test){

  test.expect(2);
  Init();
  var conn = Connect();
  test.ok(conn.connected,"Connected to database");
  var called = false;
  conn.on("fbevent", function(event,count){
    //console.log("->"+event+" "+count);
    called = true;
  });
  var eN = 'eventName';
  conn.addFBevent(eN + 1);
  conn.addFBevent(eN + 2);
  conn.deleteFBevent(eN + 2);
  conn.deleteFBevent(eN + 1);
  GenEvent(eN + 1);

  setTimeout(function(){
       test.ok(!called,"Event not called");
       conn.disconnect();
       CleanUp();
       test.done();
  }, 1000);

}
exports.AddAndDeleteMultiComplex = function(test){

  test.expect(2);
  Init();
  var conn = Connect();
  test.ok(conn.connected,"Connected to database");
  var called = false;
  conn.on("fbevent", function(event,count){
    //console.log("->"+event+" "+count);
    called = true;
  });
  var eN = 'eventName';
 /* conn.addFBevent(eN + 1);
  conn.addFBevent(eN + 2);
  conn.addFBevent(eN + 3);
  conn.addFBevent(eN + 10);
  conn.deleteFBevent(eN + 3);
  conn.deleteFBevent(eN + 2);
  conn.deleteFBevent(eN + 10);
  conn.deleteFBevent(eN + 1);
  GenEvent(eN + 1); */

  // start new
  conn.addFBevent(eN);
  var evs = [...Array(20).keys()];
  for (i in evs) {
      conn.addFBevent(eN+i);
    }
  for (i in evs) {
      conn.deleteFBevent(eN+i);
    }
  
    conn.deleteFBevent(eN);
    GenEvent(eN);
 //end new
  

  setTimeout(function(){
       test.ok(!called,"Event not called");
       conn.disconnect();
       CleanUp();
       test.done();
  }, 1000);

}


exports.WaitForManyFire2 = function(test){
    
    var expected = [
      {
          eventName: 'event1',
          genCount: 4,
          actualCount: 0
      },
      {
          eventName: 'event2',
          genCount: 0,
          actualCount: 0
      },
      {
          eventName: 'event3',
          genCount: 2,
          actualCount: 0
      }
    ], ecnt = expected.reduce(function(r,ev){ return r+ev.genCount;},0)+2;
    
    console.log('expected ',ecnt);
    test.expect(ecnt);
    Init();
    var conn = Connect();
    test.ok(conn.connected,"Connected to database");
    
    
    conn.on("fbevent", function(event,count){
        var found;
        console.log('fired event',event,count);
        expected.some(function(ev){
            if(ev.eventName==event){
                found = ev;
                return true;
            }
        });
        test.ok(found,'event fired is known to us');
        if(found) found.actualCount++;
    });
    
    var sTime=100;
    expected.forEach(function(ev){
        // wait for all events
        conn.addFBevent(ev.eventName);
        // generate events at some timeout
        for(var i=0;i<ev.genCount;i++){
             setTimeout(function(){
                console.log('fire event',ev.eventName); 
                GenEvent(ev.eventName);  
             },sTime+=1000)
        }
    });
    
   console.log('setting final setTimeout',sTime+1000);
   setTimeout(function(){
       test.ok(expected.every(function(ev){
           return ev.genCount==ev.actualCount;
       }),'all events comply');
       conn.disconnect();
       CleanUp();
       test.done();
           
   },sTime+1000); 
      
}
