/*
Copyright by Denys Khanzhiyev
See license text in LICENSE file
*/

// Load configuration
var cfg = require("../config").cfg;

// Require modules
var
  fb_binding = require("../../firebird");
var 
  testCase = require("../../tools/nodeunit").testCase;  
  
var util = {
            getDataTypeResult: function(datatype,value){
              this.conn.querySync('create table DT_TEST_TABLE ( test_field '+datatype+')');
              this.conn.querySync('insert into DT_TEST_TABLE (test_field) values ('+value+')');
              return this.conn.querySync('select first 1 test_field from DT_TEST_TABLE').fetchSync("all",false)[0][0];
            },
            quote:function(val){ return "'"+val+"'";}
         };


module.exports = testCase({
         setUp: function(callback){
           this.conn = new fb_binding.createConnection();
           this.conn.connectSync(cfg.db, cfg.user, cfg.password, cfg.role);
           callback();
         },
         tearDown: function(callback){
           this.conn.disconnect();
           this.conn = null;
           var conn = new fb_binding.createConnection();
           conn.connectSync(cfg.db, cfg.user, cfg.password, cfg.role);
           conn.querySync('drop table DT_TEST_TABLE');    
           conn.disconnect();
           callback();
         },
         char1:function(test){
            test.expect(1); 
            var Data = "Z";
            var res = util.getDataTypeResult.call(this,'char(1)',util.quote(Data));
            test.equal(res,Data,'char(1)');
            test.done();
         },
         char10:function(test){
            test.expect(1);
            var Data = "0123456789";
            var res = util.getDataTypeResult.call(this,'char(10)',util.quote(Data));
            test.equal(res,Data,'char(10)');
            test.done();
         },
         charMAX:function(test){
            test.expect(1);
            var len = 255;
            var Data = '';
            var chars = 'abcdefghABSD01234689q';
            for(var i=0; i<len;i++) Data+=chars.charAt(Math.floor(Math.random(chars.length)));
            console.log(Data.length);
            var res = util.getDataTypeResult.call(this,'char('+len+')',util.quote(Data));
            test.equal(res,Data,'char('+len+')');
            test.done();
         }
         
});