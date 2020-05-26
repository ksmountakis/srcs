#!/bin/sh
# \
exec ./tclsh $0 $*
source srcs.tcl
package require math

namespace eval srcs::sgs {
	namespace export eligseq initrule

	proc PickSample {pdf} {
		set x [math::random]
		set lo 0.0
		foreach i_pi [lsort -real -decreasing -index 1 $pdf] {
			lassign $i_pi i pi
			
			set hi [expr $lo + $pi]
			if {$x >= $lo && $x <= $hi} {
				return $i
			}
 			set lo $hi
 		}
		error "should not reach this"
	}
	
	# -- regret-based random biased sampling
	proc NexteligRBS {Y} {
		set maxvj [dict get $Y maxvj]
		
 		# set regret values r(i) = max {v(j): j in Y} - v(i) from v() 
		set rsum 0
		dict set Y rtasks [list]
 		foreach {i vi} [dict get $Y tasks] {
 			set ri [expr $maxvj - $vi]		
			dict lappend Y rtasks $i $ri
			set rsum [expr $rsum + ($ri + 1)]
		}
		
		# set probabilities p(i) = (r(i) + 1)/ sum {(r(j)+1): j in Y} from r()
 		set pdf [list]
		foreach {i ri} [dict get $Y rtasks] {
 			set pi [expr ($ri + 1.0)/$rsum]		
			lappend pdf [list $i $pi]
 		}
 		return [PickSample $pdf]
	}
	
	proc Nextelig {conf flags rule {mode ""}} {
		set n [dict get $conf n]
 		
 		# set of eligible tasks		
		set Y [dict create tasks [list] maxvj [expr -inf]]
		
		for {set m 0} {$m < $n} {incr m} {
			set j [lindex $rule $m]
			set done [dict get $flags done:$j]
			set elig [dict get $flags elig:$j]
			
			if {$done} {
				lappend Z $j $m
				continue
			}
			
			if {$elig && $mode == ""} {
				return $j
			}
			if {$elig && $mode == "RBS"} {
				dict lappend Y tasks $j $m
				dict set Y maxvj [expr max([dict get $Y maxvj], $m)]
			}
		}
		
		
		if {$mode == "" || 
			($mode == "RBS" && [llength [dict get $Y tasks]] == 0)} {
			# no more tasks to schedule
			return -1
		}
		
		# determine regret values
		return [NexteligRBS $Y]
	}
	
	proc Updateflags {conf plan flagsptr j} {
		upvar $flagsptr flags
		
		set n [dict get $conf n]
		
		dict set flags done:$j 1
		for {set z 0} {$z < $n} {incr z} {
			if { [srcs::c_plan_get_edge $plan $j $z] } {
				dict incr flags wait:$z -1
				if { [dict get $flags wait:$z] == 0 } {
					dict set flags elig:$z 1
				}
			}
		}
	}
	
	proc Initflags {conf plan} {
		set n [dict get $conf n]
 		set flags [dict create]
 		
		for {set j 0} {$j < $n} {incr j} {
			dict set flags wait:$j 0
		}
		for {set j 0} {$j < $n} {incr j} {
			dict set flags done:$j 0
			dict set flags elig:$j 0
			for {set z 0} {$z < $n} {incr z} {
				if { [srcs::c_plan_get_edge $plan $j $z] } {
					dict incr flags wait:$z
				}
			}
		}
		dict set flags elig:0 1
		
		return $flags
	}

	
	proc initrule {conf args} {
		set n [dict get $conf n]
		
		if { [llength $args] > 0 } {
			lassign $args rulepath
 			gets $rulepath rule
			if { [llength [set rule [split $rule]]] < $n } {
				error "invalid rule"
			}
		} else {
			for {set j 0} {$j < [expr $n/2]} {incr j} {
				lappend rule [expr $n-$j-1]
			}
			while { $j < $n } {
				lappend rule [expr $j - $n/2]
				incr j
			}
		}
		
		for {set j 0} {$j < $n} {incr j} {
			if { [lsearch $rule $j] == -1 } {
				error "invalid rule"
			}
		}
		
		return $rule
	}
	
	proc eligseq {conf rule plan {mode ""}} {
		set flags [Initflags $conf $plan]
		
		set eligseq [list]
		while { [set j [Nextelig $conf $flags $rule $mode]] >= 0 } {
			lappend eligseq $j
	 		Updateflags $conf $plan flags $j
		}
		
		return $eligseq
	}
	
	proc randrule {conf} {
 		set n [dict get $conf n]
		
		set rule [list 0]
		for {set p 1} {$p < $n} {incr p} {
			lappend rule -1
		}
		
		for {set j 1} {$j < $n} {incr j} {
			while { 1 } {
				set p [math::random $n]
				if { [lindex $rule $p] == -1 } {
					lset rule $p $j
					break
				}
			}
		}
		
		return $rule
	}
	
	proc sgs {conf plan eseq} {
		
		set k [expr [dict get $conf k]-1]
		set x [list]
		
 		foreach j $eseq {	
			set dj [dict get $conf $j:D:m1]
			
			# gather options and determine estj
			set opts [list 0]
			set estj 0
 			foreach pair $x {
				lassign $pair i xi
				set di [dict get $conf $i:D:m1]
				set fi [expr $xi + $di]
 				
				if {[srcs::c_plan_get_edge $plan $i $j]} {
					set estj [expr max($estj, $fi)]
				}
				if {$fi >= $estj} {
					lappend opts $fi
				}
			}
			
			# find earliest feasible option
			set opts [lsort -increasing -real $opts]
						
			for {set p 0} {$p < [llength $opts]} {incr p}  {
				# check feasibility of option p
				set t [lindex $opts $p]
				if {$t < $estj} { continue }
								
				set feas 1
				for {set r 0} {$r < $k} {incr r} {
								
					set q [dict get $conf $j:req:$r]
					if {$q == 0} {continue}
					set b [dict get $conf $r:cap]
					
					for {set p2 $p} {$p2 < [llength $opts]} {incr p2} {
						set t2 [lindex $opts $p2]
						if {$t2 >= [expr $t + $dj]} { break }
						set u [srcs::use $r $t2 $x $conf 0]						
						if {[expr $b-$u] < $q} {set feas 0; break}
					}
					if {!$feas} {break}
				}
				if {$feas} {break}
			}			
			# t is the feasible option
			if {!$feas} {error "can't schedule $j"}
			lappend x [list $j $t]
 		}

		return $x
	}
}
