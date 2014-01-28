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
           this.conn = fb_binding.createConnection();
           this.conn.connectSync(cfg.db, cfg.user, cfg.password, cfg.role);
           this.res = this.conn.querySync("select * from rdb$relations");
           callback();
         },
         tearDown: function(callback){
           this.conn.disconnect();
           callback();
         },
         FBResult: function(test){
           test.expect(1);
           test.ok(this.res,"There is result");
           test.done();
         },
         FBResultFetchSyncArgs: function(test){
           test.expect(1);
           test.throws(function(){
             this.res.fetchSync();
           },"Expect Arguments");
           test.done();
         },
         FBResFetchSyncOne: function(test){
           test.expect(1);
           var row = this.res.fetchSync("all",false);
           test.ok(row, "There is row");
           test.done();
         },
         FBResFetchMany: function(test){
           var c = 0;
           test.expect(3);
           this.res.fetch(2,false, function(row){
             c++;
             test.ok(row,"A Row");
           }, function(err,eof){
             test.ok(!err,"No Error");
             test.done(); 
           });
         }
});  


