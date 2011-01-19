/*
Copyright by Denys Khanzhiyev
See license text in LICENSE file
*/

// Load configuration
var cfg = require("../config").cfg;

// Require modules
var
  fb_binding = require("../../build/default/binding");
 

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
    console.log("this should not be called in that test "+event+' '+count ); 
    test.ok(event,"We got some event");
    test.ok(event==eName, "We got that event");
    test.ok(count==1,"One event");
  });  
  conn.addFBevent(eName);
  console.log('after add Event1');
  conn.addFBevent('another');
  console.log('after add another');
  // Wait 2 sec for event
  setTimeout(function(){
       conn.disconnect();
       CleanUp();
       test.done();    
  }, 2000);
  
}

exports.oneEvent = function(test) {
  test.expect(3);
  Init();
  
  var conn = Connect();
  test.ok(conn.connected,"Connected to database");
  var eName = "Event1";
  conn.addFBevent(eName);
  conn.addFBevent("strange");
  conn.on("fbevent",function(event,count){
    console.log("event "+event+ ' ' + count); 
    test.ok(event==eName, "We got that event");
    test.ok(count==1,"One event");
  });  
  
  console.log('before GenEvent');
  GenEvent(eName);
  console.log('after GenEvent');
  
  // Wait 2 sec for event
  setTimeout(function(){
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
  conn.addFBevent(eName);
  
  console.log('before GenEvent');
  GenEvent(eName);
  console.log('after GenEvent');

  conn.addFBevent("strange");
  conn.on("fbevent",function(event,count){
    console.log("event "+event+ ' ' + count); 
    test.ok(event==eName, "We got that event");
    test.ok(count==1,"One event");
  });  
  
  
  // Wait 2 sec for event
  setTimeout(function(){
       conn.disconnect();
       CleanUp();
       test.done();    
  }, 2000);
}


