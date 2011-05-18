var url = require('url');
var crypto = require('crypto');
var sys = require('sys');

function _hash(str){
  var h = crypto.createHash('md5');
  h.update(str);
  return h.digest('hex');
}

function _cwd(obj,name){
	if(name=='') name = 'root';
	obj.cwd = {
	  hash: _hash(name),
	  name: name,
	  mime: 'directory',
	  rel : name,
	  size: 0,
	  date: (new Date()).toString(),
	  read: true,
	  write: true,
	  rm: false
	};
}

function _cdc(obj,path){
	obj.cdc = [];
}

function _tree(path){
	return {
		hash:_hash('root'),
		name: 'root',
		read: true,
		write: true,
		dirs:[]
	}	
}

function _open(qry,cb){
   var obj = {};
   _cwd(obj,'');
   _cdc(obj,'');
   if(('tree' in qry)&& qry.tree){
	   obj.tree = _tree('');
   }	   
   cb(obj);
}

var commands = {
	open: _open	
}

function extend(obj)
{
	for(var f in obj){
	  if(obj.hasOwnProperty(f)) this[f] = obj[f];
	}
}

function elfinder(req,res){
 // console.log('in elfinder');	
  var q = url.parse(req.url,true);
  var command = 'open';
  if('cmd' in q.query) command =q.query.cmd;
  
  var add = {};
  if('init' in q.query){
   // console.log(sys.inspect(req.headers));
	add.disabled = [];
	add.params = {
			url : 'http://'+req.headers.host+'/node/elfinder',
			dotFiles: true,
			uplMaxSize: '15M',
			extract: [],
			archives: []
	};
  };
  
  if(command in commands){
	  commands[command](q.query,function(obj){
		  res.setHeader('Content-Type','application/json');
		  extend.call(obj,add);
		  res.end(JSON.stringify(obj));
	  })
  }
  else
  {
	  res.setHeader('Content-Type','text/html');
	  res.statusCode = 503;
	  res.end('Unknown command!');
  }
}

exports.elfinder = elfinder;