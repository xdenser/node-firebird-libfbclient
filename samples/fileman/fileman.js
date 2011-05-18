var connect = require('connect');
var sys = require('sys');
var elfinder = require('./elfinder').elfinder;


connect(
 function(req,res,next){
     if(req.url=='/') req.url = '/index.html';
     next();
 },
 connect.favicon(),
 connect.static(__dirname + '/static'),
 connect.bodyParser(),
 function(req,res,next){
    if(req.url.match(/^\/node/)){
      var meth = req.url.match(/^\/node\/(.+)[\?\/]/)[1];
     // console.log('method '+meth);
      var respObj = {
         elfinder: function(){
            elfinder(req, res);
         }
      };
      if(meth in respObj) respObj[meth]();
      else next();
    }
    else next();
 },
 function(req,res){
    res.setHeader('Content-Type','text/html');
    res.statusCode = 404;
    res.end("Not Found !");
 }
).listen(8080);

console.log('Server is running at http://localhost:8080');