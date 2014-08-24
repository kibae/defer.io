<?php
class DeferIO {
	private static $_conn = null;
	
	private static $cmds = [
		'auth'=>10,
		
		//'system_cmd'=>50,
		'flushcache'=>51,
		'status'=>52,
		
		//'scaleout_cmd'=>70,
		'replstream'=>71,
		'replstreamlazy'=>72,
		'repldump'=>73,
		
		'shardsetsource'=>75,
		'shardgetforce'=>76,
		
		//'object_cmd'=>90,
		'sync'=>91,
		
		'exists'=>100,			//getter
		'get'=>101,				//getter
		'getorcreate'=>102,		//[default value]
		'set'=>103,				//[new value]
		'getset'=>104,			//[new value]
		'setifnotexists'=>105,	//[new value]
		
		'arraypush'=>120,		//[value[, value, value, ...]]
			'push'=>120,
		'arraypop'=>121,		//
			'pop'=>121,
		'arrayunshift'=>122,	//[value[, value, value, ...]]
			'unshift'=>122,
		'arrayshift'=>123,		//
			'shift'=>123,
		'arraysplice'=>124,		//[off, del count[, value, value, ...]]
			'splice'=>124,		//[off, del count[, value, value, ...]]
		'arraycut'=>125,		//[size, off=0]
			'cut'=>125,
		'arrayslice'=>126,		//[size, off=0], getter
			'slice'=>126,
		'arraycount'=>127,		//getter
			'count'=>127,
		'arrayrandget'=>128,	//getter
			'randget'=>128,
		'arrayrandpop'=>129,	//
			'randpop'=>129,

		'listpush'=>140,		//[list size, value[, value, ..]], cut left
			'lpush'=>140,
		'listunshift'=>141,		//[list size, value[, value, ..]], cut right
			'lunshift'=>141,
		'listpushuniq'=>142,	//[list size, value[, value, ..]] only string/number, cut left
			'lpushuniq'=>142,
		'listunshiftuniq'=>143,	//[list size, value[, value, ..]] only string/number, cut right
			'listunshiftuniq'=>143,

		'objectset'=>160,		//[key, value[, key, value, ..]]
			'objset'=>160,
		'objectsetifnotexists'=>161,	//[key, value[, key, value, ..]]
			'objsetifnotexists'=>161,
		'objectdel'=>161,		//[key[, key, ...]]
			'objdel'=>161,
		'objectkeys'=>162,		//[search], getter
			'objkeys'=>162,

		'numberincr'=>180,		//[incr_val]
			'incr'=>180,
		'numberdecr'=>181,		//[decr_val]
			'decr'=>181,

		'stringappend'=>200,	//[right_str]
			'strappend'=>200,
		'stringprepend'=>201,	//[left_str]
			'strprepend'=>201,
		'stringlength'=>202,	//getter
			'strlength'=>202,
		'stringsub'=>203,		//[offset, len], getter
			'strsub'=>203,

		'booltoggle'=>220,		//
			'toggle'=>220,
	];
	
	private $path, $subpath=null, $options;
	public function __construct($path, $options=null) {
		$this->path = $path;
		$this->options = array_merge([/*default*/], $options?$options:[]);
		return $this;
	}
	
	public function sub($path=null) {
		$this->subpath = $path;
		return $this;
	}
	
	private $_status, $_val, $_error;
	public function status() {
		return $this->_status;
	}
	public function error() {
		return $this->_error;
	}
	public function isError() {
		return !!$this->_error;
	}
	public function val() {
		return $this->_val;
	}

	public function rval(&$to) {
		$to = $this->_val;
		return $this;
	}

	public function __call($name, $arguments) {
		$cmd = self::$cmds[strtolower($name)];
		if (!$cmd) {
			throw new Exception('Unsupported method');
			return $this;
		}
		
		$path = $this->path;
		if ($this->subpath !== null && strlen($this->subpath) > 0) {
			if (substr($this->subpath, 0, 1) == '[')
				$path .= $this->subpath;
			else
				$path .= '.'.$this->subpath;
		}
		$result = self::__request($path, $cmd, $arguments);
		var_dump($result);
		$tmp = json_decode($result[1], true);
		if ($tmp !== null)
			$result[1] = $tmp;
		$this->_status = $result[0];
		$this->_val = $result[1];
		$this->_error = $this->_status == 200 ? '' : $this->_val;

		return $this;
	}
	
	private static function __connect() {
		if (!self::$_conn) {
			self::$_conn = fsockopen('localhost', 7654);
			if (!self::$_conn)
				return false;
			//stream_set_timeout(self::$_conn, 9999999);
		}
		return self::$_conn;
	}
	
	public static function system() {
		$arguments = func_get_args();
		$cmd = array_shift($arguments);
		$cmd = self::$cmds[strtolower($cmd)];
		if (!$cmd) return false;
		return self::__request('', $cmd, $arguments);
	}
	
	private static function __response($apikey) {
		$conn = self::__connect();

		$res = fread($conn, 10);
		$header = unpack('Sstatus/Lapikey/LdataLen', $res);
		var_dump($header);
		if ($header['apikey'] != $apikey) {
			self::__disconnect();
			throw new Exception('API key error');
		}
		if ($header['dataLen'] < 0) {
			self::__disconnect();
			throw new Exception('Response error #1');
		}
		
		$buf = '';
		while (strlen($buf) < $header['dataLen']) {
			$_ = fread($conn, $header['dataLen']-strlen($buf));
			if (strlen($_) <= 0) {
				self::__disconnect();
				throw new Exception('Response error #2: disconnect');
			}
			$buf .= $_;
		}
		return [$header['status'], rtrim($buf)];
	}
	
	private static function __request($path, $cmd, $arguments=null) {
		$conn = self::__connect();
		
		$apikey = mt_rand(0, 1024*1024);
		$data = count($arguments) > 0 ? json_encode($arguments) : '';
		$buf = pack('ccLSL', $cmd, 0, $apikey, strlen($path), strlen($data)).$path.$data;
		//var_dump($buf);
		fwrite($conn, $buf);
		
		return self::__response($apikey);
	}

	private static function __disconnect() {
		if (self::$_conn) {
			fclose(self::$_conn);
			self::$_conn = null;
		}
	}
	
	public static function waitReplEntry() {
		$buf = self::__response(0);
		
		$status = $buf[0];
		if ($status == 1000) {
			$meta = unpack('Sklen/Lvlen', $buf[1]);
			$k = substr($buf[1], 6, $meta['klen']);
			$v = substr($buf[1], 6+$meta['klen'], $meta['vlen']);
			return [$k, $v];
		}else if ($status == 1010) {
			return 'finish';
		}
		return null;
	}
}
function Doc($path, $options=null) {
	return new DeferIO($path, $options);
}

?>
