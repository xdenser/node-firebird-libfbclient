/*
Copyright by Denys Khanzhiyev
See license text in LICENSE file
*/

// Load configuration
var cfg = require("../config").cfg;

// Require modules
var
  fb_binding = require("../../build/default/binding");

var zexports = {};  
 

function Init()
{
  var conn = new fb_binding.Connection;
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
  var conn = new fb_binding.Connection;
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
  process.nextTick(function loop(){
     if(finished.call()||timedout){
       clearTimeout(tid);
       clean.call();
     }
     else process.nextTick(loop);
  });
  
}

function CleanUp(){
  var con = new fb_binding.Connection;
  con.connectSync(cfg.db, cfg.user, cfg.password, cfg.role);
  con.querySync("drop procedure DoEvent;");
  con.disconnect();
}


exports.noEvent = function(test) {
  test.expect(1);
  Init();
  var conn = Connect();
  test.ok(conn.connected,"Connected to database");
  var eName = "Event1";
  conn.on("fbevent",function(event,count){
//    console.log("this should not be called in that test "+event+' '+count ); 
    test.ok(event,"We got some event");
    test.ok(event==eName, "We got that event");
    test.ok(count==1,"One event");
  });  
  conn.addFBevent(eName);
//  console.log('after add Event1');
//  conn.addFBevent('another');
//  console.log('after add another');
// Wait 1 sec for event
  setTimeout(function(){
       conn.disconnect();
       CleanUp();
       test.done();    
  }, 1000);
  
}

exports.oneEvent = function(test) {
  test.expect(3);
  Init();
  
  var conn = Connect();
  test.ok(conn.connected,"Connected to database");
  var eName = "Event1";
  var finished = false;
  conn.addFBevent(eName);
  conn.on("fbevent",function(event,count){
    test.ok(event==eName, "We got that event");
    test.ok(count==1,"One event");
    finished = true;
  });  

  GenEvent(eName);
  
  // Wait 2 sec for event
  WaitForFinish(function(){ return finished; },
  function(){
       conn.disconnect();
       CleanUp();
       test.done();    
  }, 2000);
}

exports.oneEventBetween = function(test) {
  test.expect(3);
  Init();
  
  var conn = Connect();
  test.ok(conn.connected,"Connected to database");
  var eName = "Event1";
  var finished = false;
  conn.on("fbevent",function(event,count){
    test.ok(event==eName, "We got that event");
    test.ok(count==1,"One event");
    finished = true;
  });  

  conn.addFBevent(eName);
  
  GenEvent(eName);
  
  conn.addFBevent("strange");
 
  // Wait 2 sec for event
  WaitForFinish(function(){ return finished; },
  function(){
       conn.disconnect();
       CleanUp();
       test.done();    
  }, 2000);
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
       test.ok(evcount == expected_count, "We have "+ expected_count + " events");
       conn.disconnect();
       CleanUp();
       test.done();    
  }, expected_count*250);
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