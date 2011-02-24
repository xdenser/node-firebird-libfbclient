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
            },
            JSTimeToSQLT:function(time){
              return time.getHours()+':'+time.getMinutes()+':'+time.getSeconds()+'.'+time.getMilliseconds();
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
         },
         Decimal18_2min:function(test){
            test.expect(1);
            var Data = 0.01; 
            var res = util.getDataTypeResult.call(this,'DECIMAL(18,2)',Data);
            test.equal(res,Data,'DECIMAL(18,2) min');
            test.done();
         },
         Decimal18_2max:function(test){
            test.expect(1);
            var Data = 9999999999999999.99; 
            var res = util.getDataTypeResult.call(this,'DECIMAL(18,2)',Data);
            test.equal(res,Data,'DECIMAL(18,2) max');
            test.done();
         },
         Decimal18_18min:function(test){
            test.expect(1);
            var Data = 0.000000000000000001; 
            var res = util.getDataTypeResult.call(this,'DECIMAL(18,18)',Data);
            test.equal(res,Data,'DECIMAL(18,18) min');
            test.done();
         },
         Decimal18_18max:function(test){
            test.expect(1);
            var Data = 0.999999999999999999; 
//            var Data = 1;
            var res = util.getDataTypeResult.call(this,'DECIMAL(18,18)',Data);
            test.equal(res,Data,'DECIMAL(18,18) max');
            test.done();
         },
         DoublePrecisionMin:function(test){
            test.expect(1);
            var Data = 2.225E-305; // IB DOC CLAIM  2.225E-305
            var res = util.getDataTypeResult.call(this,'DOUBLE PRECISION',Data);
            test.equal(res,Data,'DOUBLE PRECISION Min');
            test.done();
         },
	 DoublePrecisionMax:function(test){
            test.expect(1);
            var Data = 1.797E+308; 
            var res = util.getDataTypeResult.call(this,'DOUBLE PRECISION',Data);
            test.equal(res,Data,'DOUBLE PRECISION Max');
            test.done();
         },
         FloatMin:function(test){
            test.expect(1);
            var Data = 1.175E-38; 
            var res = util.getDataTypeResult.call(this,'FLOAT',Data);
            test.ok(Math.abs(res-Data)/res < 1e-3,'Float Min');
            test.done();
         },
         FloatMax:function(test){
            test.expect(1);
            var Data = 3.402E+38; 
            var res = util.getDataTypeResult.call(this,'FLOAT',Data);
            test.ok(Math.abs(res-Data)/res < 1e-3,'Float Max');
            test.done();
         },
         IntegerMin:function(test){
            test.expect(1);
            var Data = -2147483648; 
            var res = util.getDataTypeResult.call(this,'INTEGER',Data);
            test.equal(res,Data,'Integer Min');
            test.done();
         },
         IntegerMax:function(test){
            test.expect(1);
            var Data = 2147483647; 
            var res = util.getDataTypeResult.call(this,'INTEGER',Data);
            test.equal(res,Data,'Integer Max');
            test.done();
         },
         SmallIntMin:function(test){
            test.expect(1);
            var Data = -32768; 
            var res = util.getDataTypeResult.call(this,'SMALLINT',Data);
            test.equal(res,Data,'SmallInt Min');
            test.done();
         },
         SmallIntMax:function(test){
            test.expect(1);
            var Data = 32767; 
            var res = util.getDataTypeResult.call(this,'SMALLINT',Data);
            test.equal(res,Data,'SmallInt Max');
            test.done();
         },
         TimeMin:function(test){
            test.expect(1);
            var Data = new Date(0,0,0,0,0,0,0);
            var res = util.getDataTypeResult.call(this,'TIME',util.quote(util.JSTimeToSQLT(Data)));
            test.equal(res.toString(),Data.toString(),'TimeMin');
            test.done();
         },
         TimeMax:function(test){
            test.expect(1);
            var Data = new Date(0,0,0,23,59,59,999);
            var res = util.getDataTypeResult.call(this,'TIME',util.quote(util.JSTimeToSQLT(Data)));
            test.equal(res.toString(),Data.toString(),'TimeMax');
            test.done();
         },
         TimeStampMin:function(test){
            test.expect(1);
            var Data = new Date(100,0,1,0,0,0,0);
            var res = util.getDataTypeResult.call(this,'TIMESTAMP',util.quote(util.JSDateToSQLDT(Data)));
            test.equal(res.toString(),Data.toString(),'TimeStampMin');
            test.done();
         },
         TimeStampMax:function(test){
            test.expect(1);
            var Data = new Date(9999,11,31,23,59,59,999);
            var res = util.getDataTypeResult.call(this,'TIMESTAMP',util.quote(util.JSDateToSQLDT(Data)));
            test.equal(res.toString(),Data.toString(),'TimeStampMax');
            test.done();
         },
         Varchar1:function(test){
            test.expect(1); 
            var Data = "Z";
            var res = util.getDataTypeResult.call(this,'varchar(1)',util.quote(Data));
            test.equal(res,Data,'char(1)');
            test.done();
         },
         Varchar10:function(test){
            test.expect(1);
            var Data = "0123456789";
            var res = util.getDataTypeResult.call(this,'varchar(10)',util.quote(Data));
            test.equal(res,Data,'char(10)');
            test.done();
         },
         VarcharMAX:function(test){
            test.expect(1);
            var len = 32737;
            var Data = '';
            var chars = 'abcdefghABSD01234689q';
            for(var i=0; i<len;i++) Data+=chars.charAt(Math.floor(Math.random()*chars.length));
            var res = util.getDataTypeResult.call(this,'varchar('+len+')',util.quote(Data));
            test.equal(res,Data,'char('+len+')');
            test.done();
         }         
});