package require math::statistics

proc srcs::beta {dobj simmax dmin dmax {unit day}} {
	set n [llength $dmin]
	if {$unit == "day"} {
		# day in milliseconds 
		set unit [expr 24.0*60*60*1000]
	} else {
		set unit 1
	}

	set beta [dict create]
 	for {set i 0} {$i < $n} {incr i} {
		set lo [expr 1.00*[lindex $dmin $i]]
		set hi [expr 1.05*[lindex $dmax $i]]
		
		set a 2.0
		set b 1.5
		
		#set xsample [math::statistics::random-beta $a $b $simmax]
		set xsample [srcs::c_mcarlo_beta $a $b $simmax $i]
		dict set beta $i $xsample

		for {set sim 0} {$sim < $simmax} {incr sim} {
			set xi [lindex $xsample $sim]
			set di [expr $lo + $xi * ($hi - $lo)]
			# quantize to units
			set di [expr round($di / $unit)*$unit]
 			srcs::c_matr_set $dobj $sim $i real $di
		}
	}
	return $beta
}

proc srcs::matr_set_row {mobj row data {type real}} {
	for {set i 0} {$i < [llength $data]} {incr i} {
		srcs::c_matr_set $mobj $row $i $type [lindex $data $i]
	}
}

proc srcs::matr_get_row {mobj row numcols {type real}} {
	set data [list]
	for {set i 0} {$i < $numcols} {incr i} {
		lappend data [srcs::c_matr_get $mobj $row $i $type]
	}
	return $data
}
