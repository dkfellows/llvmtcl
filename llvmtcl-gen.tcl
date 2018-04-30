package require Tcl 8.6

proc val_as_obj {type value} {
    switch -exact -- $type {
	"LLVMBuilderRef" -
	"LLVMContextRef" -
	"LLVMBasicBlockRef" -
	"LLVMValueRef" -
	"LLVMTargetDataRef" -
	"LLVMTypeKind" -
	"LLVMVisibility" -
	"LLVMCallConv" -
	"LLVMOpcode" -
	"LLVMLinkage" -
	"LLVMVerifierFailureAction" -
	"LLVMIntPredicate" -
	"LLVMRealPredicate" -
	"LLVMAttribute" -
	"LLVMTypeHandleRef" -
	"LLVMTypeRef" -
	"LLVMPassManagerRef" -
	"LLVMPassManagerBuilderRef" -
	"LLVMTargetDataRef" -
	"LLVMTargetMachineRef" -
	"LLVMUseRef" -
	"LLVMGenericValueRef" -
	"LLVMModuleRef" {
	    return "Set${type}AsObj(interp, $value)"
	}
	"LLVMExecutionEngineRef *" {
	    return "Set[string trim $type { *}]AsObj(interp, $value)"
	}
	"std::string" {
	    return "Tcl_NewStringObj($value.c_str(), -1)"
	}
	"const char *" -
	"char **" {
	    return "Tcl_NewStringObj($value, -1)"
	}
	"LLVMBool" -
	"bool" -
	"int" -
	"unsigned" {
	    return "Tcl_NewIntObj($value)"
	}
	"long long" -
	"unsigned long long" {
	    return "Tcl_NewWideIntObj((Tcl_WideInt)$value)"
	}
	"double" {
	    return "Tcl_NewDoubleObj($value)"
	}
	"void" {
	}
	default {
	    upvar 1 l definition
	    return -code error \
		"Unknown return type '$type' for '$value' in '$definition'"
	}
    }
}

proc gen_api_call {cf of l} {
    lassign [split $l "("] rtnm fargs
    set rtnm [string trim $rtnm]
    set fargs [string trim $fargs]
    set rt [join [lrange [split $rtnm] 0 end-1] " "]
    set nm [lindex [split $rtnm] end]
    set fargs [string range $fargs 0 [expr {[string first ")" $fargs]-1}]]
    puts $cf "int ${nm}ObjCmd(ClientData clientData, Tcl_Interp* interp, int objc, Tcl_Obj* const objv\[\]) \{"
    set fargsl {}
    if {[string trim $fargs] ne "void"} {
	foreach farg [split $fargs ,] {
	    set farg [string trim $farg]
	    # Last part is name of argument, unless there is only one part.
	    if {[llength $farg] == 1} {
		set fargtype $farg
		set fargname ""
	    } elseif {[set idx [string last "*" $farg]] >= 0} {
		set fargtype [string range [split $farg] 0 $idx]
		set fargname [string range [split $farg] [expr {$idx+1}] end]
	    } else {
		set fargtype [lrange [split $farg] 0 end-1]
		set fargname [lindex [split $farg] end]
	    }
	    lappend fargsl $fargtype $fargname
	}
    }
    # Group types
    set pointer_types {
	"LLVMValueRef *" "LLVMTypeRef *" "LLVMGenericValueRef *"
    }
    # Check number of arguments
    set n 2
    set an 1
    set skip_next 0
    foreach {fargtype fargname} $fargsl {
	if {[string match "Out*" $fargname]} {
	    # Skip this
	} elseif {$skip_next} {
	    set skip_next 0
	} else {
	    if {$fargtype in $pointer_types} {
		if {[lindex $fargsl $n] ne "unsigned"} {
		    error "Unknown type '$fargtype' in '$l'"
		}
		set skip_next 1
	    }
	    incr an
	}
	incr n 2
    }
    set n 1
    set skip_next 0
    set wnamsg ""
    foreach {fargtype fargname} $fargsl {
	if {[string match "Out*" $fargname]} {
	    # Skip this
	} elseif {$skip_next} {
	    set skip_next 0
	} elseif {$fargtype in $pointer_types} {
	    if {[lindex $fargsl [expr {$n*2}]] ne "unsigned"} {
		error "Unknown type '$fargtype' in '$l'"
	    }
	    if {[string length $fargname]} {
		append wnamsg "$fargname " 
	    } else {
		append wnamsg "$fargtype " 
	    }
	    set skip_next 1
	} elseif {$fargname ne ""} {
	    append wnamsg "$fargname " 
	} else {
	    append wnamsg "$fargtype " 
	}
	incr n
    }
    puts $cf "    if (objc != $an) {
        Tcl_WrongNumArgs(interp, 1, objv, \"[string trim $wnamsg]\");
        return TCL_ERROR;
    }"
    # Read arguments
    set n 1
    set on 1
    set skip_next 0
    set delete_args {}
    set out_args {}
    foreach {fargtype fargname} $fargsl {
	if {[string match "Out*" $fargname]} {
	    lappend out_args arg$n $fargtype
	    switch -exact -- $fargtype {
		"LLVMExecutionEngineRef *" {
		    puts $cf "    [string trim $fargtype { *}] arg$n = nullptr; // output argument"
		}
		"char **" {
		    puts $cf "    char* arg$n = nullptr;"
		}
		default {
		    error "Unknown type '$fargtype' in '$l'"
		}
	    }
	} elseif {$skip_next} {
	    set skip_next 0
	} else {
	    switch -exact -- $fargtype {
		"LLVMExecutionEngineRef" -
		"LLVMBuilderRef" -
		"LLVMContextRef" -
		"LLVMGenericValueRef" -
		"LLVMValueRef" -
		"LLVMTargetDataRef" -
		"LLVMBasicBlockRef" -
		"LLVMTypeHandleRef" -
		"LLVMTypeRef" -
		"LLVMUseRef" -
		"LLVMPassManagerRef" -
		"LLVMPassManagerBuilderRef" -
		"LLVMTargetDataRef" -
		"LLVMTargetMachineRef" -
		"LLVMModuleRef" {
		    puts $cf "    $fargtype arg$n = nullptr;"
		    puts $cf "    if (Get${fargtype}FromObj(interp, objv\[$on\], arg$n) != TCL_OK)"
		    puts $cf "        return TCL_ERROR;"
		}
		"LLVMTypeKind" -
		"LLVMOpcode" -
		"LLVMVisibility" -
		"LLVMCallConv" -
		"LLVMIntPredicate" -
		"LLVMRealPredicate" -
		"LLVMLinkage" -
		"LLVMVerifierFailureAction" -
		"LLVMAttribute" -
		"LLVMAtomicRMWBinOp" -
		"LLVMAtomicOrdering" {
		    puts $cf "    $fargtype arg$n;"
		    puts $cf "    if (Get${fargtype}FromObj(interp, objv\[$on\], arg$n) != TCL_OK)"
		    puts $cf "        return TCL_ERROR;"
		}
		"LLVMGenericValueRef *" -
		"LLVMValueRef *" -
		"LLVMTypeRef *" {
		    if {[lindex $fargsl [expr {$n * 2}]] ne "unsigned"} {
			error "Unknown type '$fargtype' in '$l': $n, $fargsl"
		    }
		    puts $cf "    unsigned arg[expr {$n+1}] = 0;"
		    puts $cf "    $fargtype arg$n = nullptr;"
		    puts $cf "    if (GetListOf[string trim $fargtype { *}]FromObj(interp, objv\[$on\], arg$n, arg[expr {$n+1}]) != TCL_OK)"
		    puts $cf "        return TCL_ERROR;"
		    set skip_next 1
		    lappend delete_args arg$n
		}
		"std::string" -
		"const char *" {
		    puts $cf "    std::string arg$n = Tcl_GetString(objv\[$on\]);"
		}
		"LLVMBool" -
		"bool" -
		"int" {
		    puts $cf "    int arg$n = 0;"
		    puts $cf "    if (Tcl_GetIntFromObj(interp, objv\[$on\], &arg$n) != TCL_OK)"
		    puts $cf "        return TCL_ERROR;"
		}
		"uint8_t" -
		"unsigned" {
		    puts $cf "    int iarg$n = 0;"
		    puts $cf "    if (Tcl_GetIntFromObj(interp, objv\[$on\], &iarg$n) != TCL_OK)"
		    puts $cf "        return TCL_ERROR;"
		    puts $cf "    $fargtype arg$n = ($fargtype)iarg$n;"
		}
		"long long" -
		"unsigned long long" {
		    puts $cf "    Tcl_WideInt iarg$n = 0;"
		    puts $cf "    if (Tcl_GetWideIntFromObj(interp, objv\[$on\], &iarg$n) != TCL_OK)"
		    puts $cf "        return TCL_ERROR;"
		    puts $cf "    $fargtype arg$n = ($fargtype)iarg$n;"
		}
		"double" {
		    puts $cf "    double arg$n = 0;"
		    puts $cf "    if (Tcl_GetDoubleFromObj(interp, objv\[$on\], &arg$n) != TCL_OK)"
		    puts $cf "        return TCL_ERROR;"
		}
		"void" {
		}
		default {
		    error "Unknown type '$fargtype' in '$l'"
		}
	    }
	    incr on
	}
	incr n
    }
    puts -nonewline $cf "    "
    # Variable for return value
    if {$rt ne "void"} {
	puts -nonewline $cf "$rt rt = "
    }
    # Call function
    if {[info exists ::rename($nm)]} {
	puts -nonewline $cf "$::rename($nm)("
    } else {
	puts -nonewline $cf "${nm}("
    }
    set n 1
    foreach {fargtype fargname} $fargsl {
	if {$n > 1} {
	    puts -nonewline $cf ","
	}
	if {[string match "Out*" $fargname]} {
	    puts -nonewline $cf "&"
	}
	puts -nonewline $cf "arg$n"
	if {$fargtype eq "const char *"} {
	    puts -nonewline $cf ".c_str()"
	}
	incr n
    }
    puts $cf ");"
    # Return result
    if {[llength $out_args]} {
	puts $cf "    Tcl_Obj* rtl = Tcl_NewListObj(0, NULL);"
	set oa [linsert $out_args 0 rt $rt]
	foreach {rnm rtp} $oa {
	    if {$rtp ne "void"} {
		set rnm [val_as_obj $rtp $rnm]
		puts $cf "    Tcl_ListObjAppendElement(interp, rtl, $rnm);"
	    }
	}
	puts $cf "    Tcl_SetObjResult(interp, rtl);"
    } else {
	if {$rt ne "void"} {
	    set rv [val_as_obj $rt rt]
	    puts $cf "    Tcl_SetObjResult(interp, $rv);"
	}
    }
    foreach da $delete_args {
	puts $cf "    delete [] $da;"
    }
    puts $cf "    return TCL_OK;"
    puts $cf "\}"
    puts $cf ""

    set cmdnm $nm
    if {[string match "LLVM*" $cmdnm]} {
	set cmdnm [string range $cmdnm 4 end]
    }
    puts $of "    LLVMObjCmd(\"llvmtcl::$cmdnm\", ${nm}ObjCmd);"
}

proc maptokens {vals body} {
    upvar 1 pfx_len prefix sfx_len suffix
    return [join [lmap val $vals {
    	set token "\"[string tolower [string range $val $prefix end-$suffix]]\""
	string trim [subst $body] "\n"
    }] "\n"]
}
proc gen_enum {cf l} {
    set template {
int Get${nm}FromObj(Tcl_Interp* interp, Tcl_Obj* obj, $nm& e) {
    static std::map<std::string, $nm> s2e;
    if (s2e.size() == 0) {
[maptokens $vals {
        s2e\[$token\] = $val;
}]
    }
    std::string s = Tcl_GetStringFromObj(obj, 0);
    if (s2e.find(s) == s2e.end()) {
        std::ostringstream os;
        os << \"expected $nm but got \\\"\" << s << \"\\\"\";
        Tcl_SetObjResult(interp, Tcl_NewStringObj(os.str().c_str(), -1));
        return TCL_ERROR;
    }
    e = s2e\[s\];
    return TCL_OK;
}
Tcl_Obj* Set${nm}AsObj(Tcl_Interp* interp, $nm e) {
    static std::map<$nm, std::string> e2s;
    if (e2s.size() == 0) {
[maptokens $vals {
        e2s\[$val\] = $token;
}]
    }
    std::string s;
    if (e2s.find(e) == e2s.end())
        s = \"<unknown $nm>\";
    else
        s = e2s\[e\];
    return Tcl_NewStringObj(s.c_str(), -1);
}
    }

    regexp {\{(.*)\} (.*);} $l -> vals nm
    set vals [lmap val [split $vals ,] {string trim $val}]
    set nm [string trim $nm]
    set pfx_len [string length [tcl::prefix longest $vals ""]]
    set sfx_len [string length [tcl::prefix longest [lmap val $vals {
	string reverse $val
    }] ""]]
    puts $cf [string trim [subst $template]]
}

proc gen_map {mf l} {
    set template {
static std::map<std::string, $tp> ${tp}_map;
static std::map<$tp, std::string> ${tp}_refmap;
int Get${tp}FromObj(Tcl_Interp* interp, Tcl_Obj* obj, $tp& ref) {
    ref = 0;
    std::string refName = Tcl_GetStringFromObj(obj, 0);
    if (${tp}_map.find(refName) == ${tp}_map.end()) {
        std::ostringstream os;
        os << "expected $tp but got \"" << refName << "\"";
        Tcl_SetObjResult(interp, Tcl_NewStringObj(os.str().c_str(), -1));
        return TCL_ERROR;
    }
    ref = ${tp}_map[refName];
    return TCL_OK;
}
int GetListOf${tp}FromObj(Tcl_Interp* interp, Tcl_Obj* obj, $tp*& refs, unsigned& count) {
    refs = 0;
    count = 0;
    Tcl_Obj** objs = 0;
    int length;
    if (Tcl_ListObjGetElements(interp, obj, &length, &objs) != TCL_OK) {
        std::ostringstream os;
        os << "expected list of types but got \"" << Tcl_GetStringFromObj(obj, 0) << "\"";
        Tcl_SetObjResult(interp, Tcl_NewStringObj(os.str().c_str(), -1));
        return TCL_ERROR;
    }
    count = unsigned(length);
    if (count == 0)
        return TCL_OK;
    refs = new $tp[count];
    for(unsigned i = 0; i < count; i++) {
        if (Get${tp}FromObj(interp, objs[i], refs[i])) {
            delete [] refs;
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}
Tcl_Obj* Set${tp}AsObj(Tcl_Interp* interp, $tp ref) {
    if (!ref) return Tcl_NewObj();
    if (${tp}_refmap.find(ref) == ${tp}_refmap.end()) {
        std::string nm = GetRefName("${tp}_");
        ${tp}_map[nm] = ref;
        ${tp}_refmap[ref] = nm;
    }
    return Tcl_NewStringObj(${tp}_refmap[ref].c_str(), -1);
}
}

    set tp [lindex [string trim $l " ;"] end]
    puts $mf [string trim [subst -nocommands $template] "\n"]
}

set srcdir [file dirname [info script]]
if {[llength $argv] >= 1} {
    set targetdir [lindex $argv 0]
} else {
    set targetdir [file join $srcdir generic generated]
}

set f [open [file join $srcdir llvmtcl-gen.inp] r]
set ll [split [read $f] \n]
close $f

file mkdir $targetdir
set cf [open [file join $targetdir llvmtcl-gen.h] w]
set of [open [file join $targetdir llvmtcl-gen-cmddef.h] w]
set mf [open [file join $targetdir llvmtcl-gen-map.h] w]

foreach l $ll {
    set l [string trim $l]
    if {$l eq ""} continue
    switch -glob -- $l {
	"//*" { continue }
	"rename *" {
	    set rename([lindex $l 1]) [string trim [lindex $l 2] ";"]
	}
	"enum *" { gen_enum $cf $l }
	"typedef *" { gen_map $mf $l }
	default { gen_api_call $cf $of $l }
    }
}

close $cf
close $of
close $mf
