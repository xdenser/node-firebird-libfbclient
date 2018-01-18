/*
Copyright by Denys Khanzhiyev
See license text in LICENSE file
*/

// Load configuration
var cfg = require("../config").cfg;

// Require modules
var
  fb_binding = require("../../firebird.js");
  
 

exports.SyncConnection = function (test) {
  test.expect(3);
  var conn = fb_binding.createConnection();
  conn.connectSync(cfg.db, cfg.user, cfg.password, cfg.role);
  test.ok(conn.connected,"Connected to database");
  var res = conn.querySync("select * from rdb$relations");
  test.ok(res,"Can query");
  conn.disconnect();
  test.ok(!conn.connected,"Disconnected to database");
  test.done();
};

function Connect(){
  var conn = fb_binding.createConnection();
  conn.connectSync(cfg.db, cfg.user, cfg.password, cfg.role);
  conn.querySync("create table TEST_TRANS (ID INTEGER, NAME VARCHAR(20))");
  var sql =  'create or alter procedure TEST_PROC(\n'+
               '"INPUT" varchar(25))\n'+
               'returns (\n'+
               '"OUTPUT" varchar(25))\n'+
               'as\n'+
               'BEGIN\n'+
               '  output = input;\n'+
               '  suspend;\n'+
               'END\n';
  conn.querySync(sql);
  return conn;
}
function Close(conn){
  conn.disconnect();
  var con = fb_binding.createConnection();
  con.connectSync(cfg.db, cfg.user, cfg.password, cfg.role);
  con.querySync("drop table TEST_TRANS;");
  con.querySync('drop procedure TEST_PROC');
  con.disconnect();
}

exports.SyncDDLTransaction = function(test) {
  test.expect(2);
  var conn = Connect();
  test.ok(conn.connected,"Connected to database");
  test.ok(!conn.inTransaction,"DDL should commit automatically");
  Close(conn);    
  test.done();
}

exports.SyncTransCommit = function(test) {
  test.expect(5);
  var conn = Connect();
  test.ok(conn.connected,"Connected to database");
  
  test.doesNotThrow(function(){
    conn.querySync("insert into TEST_TRANS (ID, NAME) values (0, 'Test Name 1')");
  },"insert query");
      
  test.ok(conn.inTransaction,"DML should not commit automatically");
  
  test.doesNotThrow(function(){
   conn.commitSync();
  },"commit insert"); 
  
  test.ok(!conn.inTransaction,"Not in transaction");
  Close(conn);    
  test.done();
}

exports.SyncTransRollback = function(test) {
  test.expect(6);
  var conn = Connect();
  test.ok(conn.connected,"Connected to database");
  
  test.doesNotThrow(function(){
    conn.querySync("insert into TEST_TRANS (ID, NAME) values (0, 'Test Name 1')");
  },"insert query");
  
  var res;
  test.doesNotThrow(function(){
  res = conn.querySync("select * from TEST_TRANS").fetchSync("all",true);
  },"select before rollback");
  
  test.ok(res.length == 1, "Got one result");
  
  conn.rollbackSync();
  
  var res;
  test.doesNotThrow(function(){
  res = conn.querySync("select * from TEST_TRANS").fetchSync("all",true);
  },"select after rollback");
  
  test.ok(res.length == 0, "Got zero result");
  
  Close(conn);
  test.done();
}



exports.SyncQueryFromSP = function(test) {
  test.expect(4);
  var conn = Connect();
  test.ok(conn.connected,"Connected to database");
  var res;
  test.doesNotThrow(function(){
	res = conn.querySync("select * from TEST_PROC('TEST STRING')").fetchSync("all",true);
  },"select from store proc");
  test.ok(res.length == 1, "Got one result");
  test.equal(res[0].OUTPUT, 'TEST STRING','returned data equal'); 
  Close(conn);    
  test.done();
}


