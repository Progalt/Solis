// Generated automatically from F:\Dev\Solis\Solis\core.solis. Do not edit.
static const char* coreModuleSource =
"\n"
"class Number\n"
"\n"
"	function abs()\n"
"		\n"
"		if self < 0 then \n"
"			return -self\n"
"		end \n"
"\n"
"		return self\n"
"\n"
"	end\n"
"\n"
"end\n"
"\n"
"class Bool\n"
"\n"
"	function toString()\n"
"		\n"
"		if self then \n"
"			return \"true\"\n"
"		end\n"
"\n"
"		return \"false\"\n"
"	end	\n"
"\n"
"end \n"
"\n"
"class String\n"
"	\n"
"	function toString()\n"
"		return self\n"
"	end\n"
"\n"
"end\n"
"\n"
"class List\n"
"\n"
"	function toString()\n"
"		var str = \"[ \"\n"
"		var idx = 0\n"
"\n"
"		var len = self.length()\n"
"		while idx < len do\n"
"			\n"
"			var idxStr = self.at(idx).toString()\n"
"			str = str + idxStr\n"
"\n"
"			if idx < len - 1 then\n"
"				str = str + \", \"\n"
"			end\n"
"\n"
"			idx = idx + 1\n"
"		end\n"
"\n"
"		str = str + \" ]\"\n"
"\n"
"		return str\n"
"\n"
"	end\n"
"\n"
"	function iterate(itr)\n"
"		if itr == null then\n"
"			return 0\n"
"		end\n"
"\n"
"		var i = itr\n"
"		if itr >= self.length() - 1 then\n"
"			return false\n"
"		end\n"
"\n"
"		return i + 1 \n"
"	end \n"
"\n"
"	function iteratorValue(itr)\n"
"\n"
"		return self[itr]\n"
"\n"
"	end\n"
"\n"
"end\n"
" \n"
"class Range\n"
"\n"
"	var min = 0\n"
"	var max = 0\n"
"	var step = 1\n"
"\n"
"	function toString()\n"
"\n"
"		var str = self.min.toString()\n"
"		str = str + \"..\" \n"
"		str = str + self.max.toString()\n"
"\n"
"		return str\n"
"	end\n"
"\n"
"	function expand()\n"
"		var out = []\n"
"		var idx = self.min\n"
"\n"
"		while idx < self.max do\n"
"\n"
"			out.append(idx)\n"
"			idx = idx + self.step\n"
"		end\n"
"\n"
"		return out\n"
"\n"
"	end\n"
"\n"
"	function iterate(itr)\n"
"\n"
"		if itr == null then\n"
"			return 0\n"
"		end\n"
"\n"
"		var i = itr\n"
"		var maxVal = ((self.max - self.step) - self.min) / self.step\n"
"		if itr >= maxVal then\n"
"			return false\n"
"		end\n"
"\n"
"		return i + 1\n"
"\n"
"	end\n"
"\n"
"	function iteratorValue(index)\n"
"		\n"
"		return (self.min + (index * self.step))\n"
"	end\n"
"\n"
"end\n"
"\n"
"\n"
"function println(val)\n"
"	\n"
"	__c_printf(val.toString())\n"
"\n"
"end";