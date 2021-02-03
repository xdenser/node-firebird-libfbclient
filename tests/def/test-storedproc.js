/*
Copyright by Denys Khanzhiyev
See license text in LICENSE file
*/

// Load configuration
var cfg = require("../config").cfg;

// Require modules
var
  fb_binding = require("../../firebird.js");
  
var 
  testCase = require("nodeunit").testCase;  

  module.exports = testCase({
    setUp: function(callback){
      this.conn = new fb_binding.createConnection();
      this.conn.connectSync(cfg.db, cfg.user, cfg.password, cfg.role);
      var sql = 'CREATE OR ALTER PROCEDURE TESTINGPROCEDURE (TESTINPUT BIGINT) RETURNS ( TESTOUTPUT BIGINT ) AS declare variable CON integer; declare variable MAXCON integer; begin TESTOUTPUT = TESTINPUT; end';
      this.conn.querySync(sql);
      callback();
    },
    tearDown: function(callback){
      this.conn.disconnect();
      this.conn = null;
      var conn = new fb_binding.createConnection();
      conn.connectSync(cfg.db, cfg.user, cfg.password, cfg.role);
      conn.querySync('drop procedure TESTINGPROCEDURE');    
      conn.disconnect();
      callback();
    },
    testProc : function(test)  {
        test.expect(2);
                    
        const query = "EXECUTE PROCEDURE TESTINGPROCEDURE (1);"
    
        this.conn.startNewTransaction((err, transaction) => {
            transaction.query(query, (err, result) => {
                if(err) {
                    console.log(err);
                }    
                test.ok(!err, "error on query");
                transaction.commit(err => {
                    //console.log(result);
                    test.equal(result.TESTOUTPUT, 1, "wrong result");
                    test.done();
                })    
            });
        });
        
    }
});