namespace eval llvmtcl {
    namespace export *
    namespace ensemble create
    namespace eval DebugInfo {
	namespace export *
	namespace ensemble create
    }

    proc OptimizeModule {m optimizeLevel} {
	set pm [CreatePassManager]
	set bld [PassManagerBuilderCreate]
	PassManagerBuilderSetOptLevel $bld $optimizeLevel
	PassManagerBuilderSetDisableUnrollLoops $bld [expr {$optimizeLevel == 0}]
	if {$optimizeLevel > 1} {
	    PassManagerBuilderSetDisableUnrollLoops $bld 0
	    PassManagerBuilderUseInlinerWithThreshold $bld [expr {$optimizeLevel > 2 ? 275 : 225 }]
	}
	PassManagerBuilderPopulateModulePassManager $bld $pm
	RunPassManager $pm $m
	DisposePassManager $pm
    }

    proc OptimizeFunction {m f optimizeLevel} {
	set fpm [CreateFunctionPassManagerForModule $m]
	set bld [PassManagerBuilderCreate]
	PassManagerBuilderSetOptLevel $bld $optimizeLevel
	PassManagerBuilderSetDisableUnrollLoops $bld [expr {$optimizeLevel == 0}]
	if {$optimizeLevel > 1} {
	    PassManagerBuilderSetDisableUnrollLoops $bld 0
	    PassManagerBuilderUseInlinerWithThreshold $bld [expr {$optimizeLevel > 2 ? 275 : 225 }]
	}
	PassManagerBuilderPopulateFunctionPassManager $bld $fpm
	InitializeFunctionPassManager $fpm
	RunFunctionPassManager $fpm $f
	FinalizeFunctionPassManager $fpm
	DisposePassManager $fpm
    }

    proc Optimize {m funcList} {
	foreach f $funcList {
	    OptimizeFunction $m $f 3
	}
	OptimizeModule $m 3
    }

    proc Execute {m f args} {
	SetTarget $m X86
	lassign [CreateExecutionEngineForModule $m] rt EE msg
	set largs {}
	foreach arg $args {
	    lappend largs [CreateGenericValueOfInt [Int32Type] $arg 0]
	}
	set rt [GenericValueToInt [RunFunction $EE $f $largs] 0]
    }

    proc Tcl2LLVM {m procName {functionDeclarationOnly 0}} {
	variable tstp
	variable ts
	variable tsp
	variable funcar
	variable utils_added

	set i32 [Int32Type]
	set i8 [Int8Type]

	if {![info exists utils_added($m)] || !$utils_added($m)} {
	    AddTcl2LLVMUtils $m
	}
	# Disassemble the proc
	set dasm [split [tcl::unsupported::disassemble proc $procName] \n]
	# Create builder
	set bld [CreateBuilder]
	# Create function
	if {![info exists funcar($m,$procName)]} {
	    set argl {}
	    foreach l $dasm {
		set l [string trim $l]
		if {[regexp {slot \d+, .*arg, \"} $l]} {
		    lappend argl $i32
		}
	    }
	    set ft [FunctionType $i32 $argl 0]
	    set func [AddFunction $m $procName $ft]
	    set funcar($m,$procName) $func
	}
	if {$functionDeclarationOnly} {
	    return $funcar($m,$procName)
	}
	set func $funcar($m,$procName)
	# Create basic blocks
	set block(0) [AppendBasicBlock $func "block0"]
	set next_is_ipath -1
	foreach l $dasm {
	    set l [string trim $l]
	    if {![string match "(*" $l]} { continue }
	    set opcode [lindex $l 1]
	    if {$next_is_ipath >= 0} {
		regexp {\((\d+)\) } $l -> pc
		if {![info exists block($pc)]} {
		    set block($pc) [AppendBasicBlock $func "block$pc"]
		}
		set ipath($next_is_ipath) $pc
		set next_is_ipath -1
	    }
	    if {[string match "jump*1" $opcode] || [string match "jump*4" $opcode] || [string match "startCommand" $opcode]} {
		# (pc) opcode offset
		regexp {\((\d+)\) (jump\S*[14]|startCommand) (\+*\-*\d+)} $l -> pc cmd offset
		set tgt [expr {$pc + $offset}]
		if {![info exists block($tgt)]} {
		    set block($tgt) [AppendBasicBlock $func "block$tgt"]
		}
		set next_is_ipath $pc
	    }
	}
	PositionBuilderAtEnd $bld $block(0)
	set curr_block $block(0)
	# Create stack and stack pointer
	set tstp [PointerType $i8 0]
	set at [ArrayType [PointerType $i8 0] 1000]
	set ts [BuildArrayAlloca $bld $at [ConstInt $i32 1 0] ""]
	set tsp [BuildAlloca $bld $i32 ""]
	BuildStore $bld [ConstInt $i32 0 0] $tsp
	# Load arguments into llvm, allocate space for slots
	set n 0
	foreach l $dasm {
	    set l [string trim $l]
	    if {[regexp {slot \d+, .*arg, \"} $l]} {
		set arg_1 [GetParam $func $n]
		set arg_2 [BuildAlloca $bld $i32 ""]
		set arg_3 [BuildStore $bld $arg_1 $arg_2]
		set vars($n) $arg_2
		incr n
	    } elseif {[string match "slot *" $l]} {
		set arg_2 [BuildAlloca $bld $i32 ""]
		set vars($n) $arg_2
	    }
	}
	# Convert Tcl parse output
	set binaryOp(bitor) BuildXor
	set binaryOp(bitxor) BuildOr
	set binaryOp(bitand) BuildAnd
	set binaryOp(lshift) BuildShl
	set binaryOp(rshift) BuildAShr
	set binaryOp(add) BuildAdd
	set binaryOp(sub) BuildSub
	set binaryOp(mult) BuildMul
	set binaryOp(div) BuildSDiv
	set binaryOp(mod) BuildSRem

	set unaryOp(uminus) BuildNeg
	set unaryOp(bitnot) BuildNot

	set comparison(eq) LLVMIntEQ
	set comparison(neq) LLVMIntNE
	set comparison(lt) LLVMIntSLT
	set comparison(gt) LLVMIntSGT
	set comparison(le) LLVMIntSLE
	set comparison(ge) LLVMIntGE

	set done_done 0
	foreach l $dasm {
	    #puts $l
	    set l [string trim $l]
	    if {![string match "(*" $l]} { continue }
	    regexp {\((\d+)\) (\S+)} $l -> pc opcode
	    if {[info exists block($pc)]} {
		PositionBuilderAtEnd $bld $block($pc)
		set curr_block $block($pc)
		set done_done 0
	    }
	    set ends_with_jump($curr_block) 0
	    unset -nocomplain tgt
	    if {[string match "jump*1" $opcode] || [string match "jump*4" $opcode] || [string match "startCommand" $opcode]} {
		regexp {\(\d+\) (?:jump\S*[14]|startCommand) (\+*\-*\d+)} $l -> offset
		set tgt [expr {$pc + $offset}]
	    }
	    if {[info exists unaryOp($opcode)]} {
		push $bld [$unaryOp($opcode) $bld [pop $bld $i32] ""]
	    } elseif {[info exists binaryOp($opcode)]} {
		set top0 [pop $bld $i32]
		set top1 [pop $bld $i32]
		push $bld [$binaryOp($opcode) $bld $top1 $top0 ""]
	    } elseif {[info exists comparison($opcode)]} {
		set top0 [pop $bld $i32]
		set top1 [pop $bld $i32]
		push $bld [BuildIntCast $bld [BuildICmp $bld $comparison($opcode) $top1 $top0 ""] $i32 ""]
	    } else {
		switch -exact -- $opcode {
		    "loadScalar1" {
			set var $vars([string range [lindex $l 2] 2 end])
			push $bld [BuildLoad $bld $var ""]
		    }
		    "storeScalar1" {
			set var_1 [top $bld $i32]
			set idx [string range [lindex $l 2] 2 end]
			if {[info exists vars($idx)]} {
			    set var_2 $vars($idx)
			} else {
			    set var_2 [BuildAlloca $bld $i32 ""]
			}
			set var_3 [BuildStore $bld $var_1 $var_2]
			set vars($idx) $var_2
		    }
		    "incrScalar1" {
			set var $vars([string range [lindex $l 2] 2 end])
			BuildStore $bld [BuildAdd $bld [BuildLoad $bld $var ""] [top $bld $i32] ""] $var
		    }
		    "incrScalar1Imm" {
			set var $vars([string range [lindex $l 2] 2 end])
			set i [lindex $l 3]
			set s [BuildAdd $bld [BuildLoad $bld $var ""] [ConstInt $i32 $i 0] ""]
			push $bld $s
			BuildStore $bld $s $var
		    }
		    "push1" {
			set tval [lindex $l 4]
			if {[string is integer -strict $tval]} {
			    set val [ConstInt $i32 $tval 0]
			} elseif {[info exists funcar($m,$tval)]} {
			    set val $funcar($m,$tval)
			} else {
			    set val [ConstInt $i32 0 0]
			}
			push $bld $val
		    }
		    "jumpTrue4" -
		    "jumpTrue1" {
			set top [pop $bld $i32]
			if {[GetIntTypeWidth [TypeOf $top]] == 1} {
			    set cond $top
			} else {
			    set cond [BuildICmp $bld LLVMIntNE $top [ConstInt $i32 0 0] ""]
			}
			BuildCondBr $bld $cond $block($tgt) $block($ipath($pc))
			set ends_with_jump($curr_block) 1
		    }
		    "jumpFalse4" -
		    "jumpFalse1" {
			set top [pop $bld $i32]
			if {[GetIntTypeWidth [TypeOf $top]] == 1} {
			    set cond $top
			} else {
			    set cond [BuildICmp $bld LLVMIntNE $top [ConstInt $i32 0 0] ""]
			}
			BuildCondBr $bld $cond $block($ipath($pc)) $block($tgt)
			set ends_with_jump($curr_block) 1
		    }
		    "tryCvtToNumeric" {
			push $bld [pop $bld $i32]
		    }
		    "startCommand" {
		    }
		    "jump4" -
		    "jump1" {
			BuildBr $bld $block($tgt)
			set ends_with_jump($curr_block) 1
		    }
		    "invokeStk1" {
			set objc [lindex $l 2]
			set objv {}
			set argl {}
			for {set i 0} {$i < ($objc-1)} {incr i} {
			    lappend objv [pop $bld $i32]
			    lappend argl $i32
			}
			set objv [lreverse $objv]
			set ft [PointerType [FunctionType $i32 $argl 0] 0]
			set fptr [pop $bld $ft]
			push $bld [BuildCall $bld $fptr $objv ""]
		    }
		    "pop" {
			pop $bld $i32
		    }
		    "done" {
			if {!$done_done} {
			    BuildRet $bld [top $bld $i32]
			    set ends_with_jump($curr_block) 1
			    set done_done 1
			}
		    }
		    "nop" {
		    }
		    default {
			error "unknown bytecode '$opcode' in '$l'"
		    }
		}
	    }
	}
	# Set increment paths
	foreach {pc b} [array get block] {
	    PositionBuilderAtEnd $bld $block($pc)
	    if {![info exists ends_with_jump($block($pc))] || !$ends_with_jump($block($pc))} {
		set tpc [expr {$pc+1}]
		while {$tpc < 1000} {
		    if {[info exists block($tpc)]} {
			BuildBr $bld $block($tpc)
			break
		    }
		    incr tpc
		}
	    }
	}
	# Cleanup and return
	DisposeBuilder $bld
	return $func
    }

    # Helper functions, should not be called directly

    proc AddTcl2LLVMUtils {m} {
	variable funcar
	variable utils_added
	set bld [CreateBuilder]
	set ft [FunctionType [Int32Type] [list [Int32Type]] 0]
	set func [AddFunction $m "llvm_mathfunc_int" $ft]
	set funcar($m,tcl::mathfunc::int) $func
	set block [AppendBasicBlock $func "block"]
	PositionBuilderAtEnd $bld $block
	BuildRet $bld [GetParam $func 0]
	DisposeBuilder $bld
	set utils_added($m) 1
    }

    proc push {bld val} {
	variable tstp
	variable ts
	variable tsp
	# Allocate space for value
	set valt [TypeOf $val]
	set valp [BuildAlloca $bld $valt "push"]
	BuildStore $bld $val $valp
	# Store location on stack
	set tspv [BuildLoad $bld $tsp "push"]
	set tsl [BuildGEP $bld $ts [list [ConstInt [Int32Type] 0 0] $tspv] "push"]
	BuildStore $bld [BuildPointerCast $bld $valp $tstp ""] $tsl
	# Update stack pointer
	set tspv [BuildAdd $bld $tspv [ConstInt [Int32Type] 1 0] "push"]
	BuildStore $bld $tspv $tsp
    }
    
    proc pop {bld valt} {
	variable ts
	variable tsp
	# Get location from stack and decrement the stack pointer
	set tspv [BuildLoad $bld $tsp "pop"]
	set tspv [BuildAdd $bld $tspv [ConstInt [Int32Type] -1 0] "pop"]
	BuildStore $bld $tspv $tsp
	set tsl [BuildGEP $bld $ts [list [ConstInt [Int32Type] 0 0] $tspv] "pop"]
	set valp [BuildLoad $bld $tsl "pop"]
	# Load value
	set pc [BuildPointerCast $bld $valp [PointerType $valt 0] "pop"]
	set rt [BuildLoad $bld $pc "pop"]
	return $rt
    }
    
    proc top {bld valt {offset 0}} {
	variable ts
	variable tsp
	# Get location from stack
	set tspv [BuildLoad $bld $tsp "top"]
	set tspv [BuildAdd $bld $tspv [ConstInt [Int32Type] -1 0] "top"]
	set tsl [BuildGEP $bld $ts [list [ConstInt [Int32Type] 0 0] $tspv] "top"]
	set valp [BuildLoad $bld $tsl "top"]
	# Load value
	return [BuildLoad $bld [BuildPointerCast $bld $valp [PointerType $valt 0] "top"] "top"]
    }
}
