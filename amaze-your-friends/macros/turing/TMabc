

START{####:####:R:check}
check{a:a:N:find_b}{b:REJECT:N:HALT}{c:REJECT:N:HALT}{%%%%:ACCEPT:N:HALT}
find_b{a:a:R:find_b}{b:b:L:kill_a1}{c:REJECT:N:HALT}{%%%%:REJECT:N:HALT}
kill_a1{a:b:R:find_c}
find_c{b:b:R:find_c}{c:c:L:kill_b2}{%%%%:REJECT:N:HALT}
kill_b2{b:c:L:kill_b1}
kill_b1{b:c:R:find_end}
find_end{c:c:R:find_end}{%%%%:%%%%:L:kill_c3}
kill_c3{c:%%%%:L:kill_c2}
kill_c2{c:%%%%:L:kill_c1}
kill_c1{c:%%%%:L:RETURN}
RETURN{a:a:L:RETURN}{b:b:L:RETURN}{c:c:L:RETURN}{####:####:N:START}

####
a
a
b
b
c
c
%%%%
