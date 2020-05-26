package require math::statistics

#
# make uniform distributions,
# make monte carlo sample
#

proc srcs::uni_d_new {dobj simmax conf {mode low}} {
	set n [dict get $conf n]
	
 	for {set i 0} {$i < $n} {incr i} {
		set m1 [dict get $conf $i:D:m1]
		if {$m1 == 0} {continue}
		
		switch $mode {
			low {
				set tlo [expr $m1-sqrt($m1)]
				set thi [expr $m1+sqrt($m1)]
			}
			
			high {
				set tlo [expr $m1-$m1]
				set thi [expr $m1+$m1]
			}
		}
			
		set t [math::statistics::random-uniform $tlo $thi $simmax]

		srcs::c_matr_set $dobj 0 $i real $m1
		for {set m 1} {$m < $simmax} {incr m} {
			set ti [lindex $t $m]
 			srcs::c_matr_set $dobj $m $i real $ti
		}
	}
}

#
# make beta distributions,
# make monte carlo sample
#

proc srcs::bet_d_new {dobj simmax d {mode low}} {
	set n [llength $d]
 	for {set i 0} {$i < $n} {incr i} {
		set m1 [lindex $d $i]
		if {$m1 == 0} {continue}
		
		# adjust parameters `a' and `b'
		switch $mode {
			low {
				set a [expr 0.5*$m1 - (1.0/3.0)];
			}
			
			high {
				set a [expr (1.0/6.0)];
			}
		}
		set b [expr $a*2.0]
		
		if {$a < 1 || $b < 1} {
			puts stderr "a < 1 or b < 1"
		}
					
		set xsample [math::statistics::random-beta $a $b $simmax]

		srcs::c_matr_set $dobj 0 $i real $m1
		for {set m 1} {$m < $simmax} {incr m} {
			set xi [lindex $xsample $m]
			set ti [expr (3.0/2.0)*$m1*$xi + 0.5*$m1]
 			srcs::c_matr_set $dobj $m $i real $ti
		}
	}
}

#
# make beta distributions,
#

proc srcs::vilches_d_new {dobj simmax conf mode {dis 1} {d0type mean}} {
	set n [dict get $conf n]
	
	set a 2
	set b 5
	if {$mode == "low"} {
		set lo 0.75
		set hi 1.625
	}
	if {$mode == "med"} {
		set lo 0.5
		set hi 2.25
	}
	if {$mode == "high"} {
		set lo 0.25
		set hi 2.875
	}
	
 	for {set i 0} {$i < $n} {incr i} {
		set m1 [dict get $conf $i:D:m1]
		if {$m1 == 0} {continue}
 				
		set xsample [math::statistics::random-beta $a $b $simmax]

		if {$d0type == "mean"} {
			srcs::c_matr_set $dobj 0 $i real $m1
		}
		if {$d0type == "mode"} {
			set xi [expr ($a - 1.0)/($a + $b - 2.0)]
			set di [expr $m1*($lo + ($hi-$lo)*$xi)]
			if {$dis == 1} {
				set di [expr int($di)]
			}
 			srcs::c_matr_set $dobj 0 $i real $di
		}
		
		for {set m 1} {$m < $simmax} {incr m} {
			set xi [lindex $xsample $m]
			set di [expr $m1*($lo + ($hi-$lo)*$xi)]
			if {$dis == 1} {
				set di [expr int($di)]
			}
 			srcs::c_matr_set $dobj $m $i real $di
		}
	}
}


proc srcs::exp_d_new {dobj simmax conf} {
	set n [dict get $conf n]
	
	for {set i 0} {$i < $n} {incr i} {
		set m1 [dict get $conf $i:D:m1]
		if {$m1 == 0} {continue}
			
		set t [math::statistics::random-exponential $m1 $simmax]

		srcs::c_matr_set $dobj 0 $i real $m1
		for {set m 1} {$m < $simmax} {incr m} {
			set ti [lindex $t $m]
 			srcs::c_matr_set $dobj $m $i real $ti
		}
	}
}

#
# only 20% of tasks random
#

proc srcs::few_exp_d_new {dobj simmax conf {pct 0.5}} {
	set n [dict get $conf n]
	
	for {set i 0} {$i < $n} {incr i} {
		set m1 [dict get $conf $i:D:m1]
		if {$m1 == 0} {continue}
		
		if { [math::random] <= $pct } {
			set rnd 1
		} else {
			set rnd 0
		}
		
		# t[] = column of random durations for i
		set t [list]
		for {set m 0} {$m < $simmax} {incr m} {
			set ti $m1
			if {$rnd && [math::random] <= 0.5} {
				set ti 0
			}
			lappend t $ti
		}
		
		srcs::c_matr_set $dobj 0 $i real [expr $m1*0.5 + 0*0.5]
		for {set m 1} {$m < $simmax} {incr m} {
			set ti [lindex $t $m]
 			srcs::c_matr_set $dobj $m $i real $ti
		}
	}
}

#
# correlated durations
#

proc srcs::cor_d_new {dobj simmax conf} {
	set n [dict get $conf n]
	
 	for {set i 0} {$i < $n} {incr i} {
		set m1 [dict get $conf $i:D:m1]
		if {$m1 == 0} {continue}
			
		# set t [math::statistics::random-uniform $tlo $thi $simmax]

		srcs::c_matr_set $dobj 0 $i real $m1
		for {set m 1} {$m < $simmax} {incr m} {
			set ti [lindex $t $m]
 			srcs::c_matr_set $dobj $m $i real $ti
		}
	}
}

proc srcs::dur_load {mat dfile conf {alloc ""} {strict ""}} {
 	set durdata [split [string trim [read $dfile]] "\n"]
 	
 	set n [dict get $conf n]
 	if {$alloc == "alloc"} {	
 		set simmax [llength $durdata]
 		srcs::c_matr_new $mat $simmax $n
 	} else {
		set simmax [expr min([llength $durdata], [srcs::c_matr_get_rows $mat])]
	}
  	
  	if {$simmax < 1 || [srcs::c_matr_get_cols $mat] != $n} {
  		error "$mat: invalid dimensions"
  	}
   	
 	# fill matrix
	set rowcount 0
	foreach row $durdata {
 		if {$rowcount == $simmax} {
			break
		}
		
		if {[llength $row] != $n} {
			error "len(row $row) = [llength $row] != $n"
 		}
		
		for {set i 0} {$i < $n} {incr i} {
			if { ![string is double [set di [lindex $row $i]]] } {
				error "durfile (line $rowcount): $di is not double"
 			}
			if {$strict == "strict" && $rowcount == 0 && $di != [dict get $conf $i:D:m1] } {
				error "dfile (line $rowcount): $di != mean(D$i)"
 			}
			srcs::c_matr_set $mat $rowcount $i real $di
		}
		
		incr rowcount
	}
 	return $simmax
}
