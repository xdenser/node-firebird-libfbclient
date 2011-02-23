/*
Copyright by Denys Khanzhiyev
See license text in LICENSE file
*/

// Load configuration
var cfg = require("../config").cfg;
var sys = require("sys");

// Require modules
var
  fb_binding = require("../../firebird");
var 
  testCase = require("../../tools/nodeunit").testCase;  
  
var util = {
            getDataTypeResult: function(datatype,value){
              this.conn.querySync('create table DT_TEST_TABLE ( test_field '+datatype+')');
              this.conn.querySync('insert into DT_TEST_TABLE (test_field) values ('+value+')');
              this.conn.commitSync();
              return this.conn.querySync('select first 1 test_field from DT_TEST_TABLE').fetchSync("all",false)[0][0];
            },
            quote:function(val){ return "'"+val+"'";},
            JSDateToSQLDT: function(date){
               return date.getFullYear()+'-'+(date.getMonth()+1)+'-'+date.getDate()+' '+date.getHours()+':'+date.getMinutes()+':'+date.getSeconds()+'.'+date.getMilliseconds();
            },
            JSDateToSQLD: function(date){
               return date.getFullYear()+'-'+(date.getMonth()+1)+'-'+date.getDate();
            }
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
            var len = 32737;
            var Data = '';
            var chars = 'abcdefghABSD01234689q';
            for(var i=0; i<len;i++) Data+=chars.charAt(Math.floor(Math.random()*chars.length));
            var res = util.getDataTypeResult.call(this,'char('+len+')',util.quote(Data));
            test.equal(res,Data,'char('+len+')');
            test.done();
         },
         DateMin:function(test){
            test.expect(1);
            var Data = new Date(100,0,1,0,0,0,0);
            var res = util.getDataTypeResult.call(this,'DATE',util.quote(util.JSDateToSQLD(Data)));
            test.equal(res.toString(),Data.toString(),'DateMin');
            test.done();
         },
         DateToday:function(test){
            test.expect(1);
            var Data = new Date();
            Data.setHours(0);
            Data.setMinutes(0);
            Data.setSeconds(0);
            Data.setMilliseconds(0);
            var res = util.getDataTypeResult.call(this,'DATE',util.quote(util.JSDateToSQLD(Data)));
            test.equal(res.toString(),Data.toString(),'DateToday');
            test.done();
         },
         DateMax:function(test){
            test.expect(1);
            var Data = new Date(9999,11,31,0,0,0,0); // Interbase documentation claims max value for year is 32768, but insert with 32768-2-29 throws error 
            var res = util.getDataTypeResult.call(this,'DATE',util.quote(util.JSDateToSQLD(Data)));
            test.equal(res.toString(),Data.toString(),'DateMax');
            test.done();
         }
         
});