$z = |2,1|[1., 2.]
$m = |1,2|[1., 2.] dot $z
$mc = |1,1|[5.]
$first = |3,1|[1.,2.,3.]
$second = |3,1|[1., 2., 3.]
$final = $first + $second
$finalCheck = |3,1|[1.,4.,6.]
$beforeRelu = |2,2|[-3.0, -5.5, 3., 8.]
$afterRelu = relu(-$beforeRelu)
$reluCheck = |2,2|[3.0, 5.5, 0., 0.]
.plot $m 0.0 10.
.plot $mc 0.0 10.0
.plot $final 0.0 10.0
.plot $finalCheck 0.0 10.0
.plot $beforeRelu 0.0 10.0
.plot $afterRelu 0.0 10.0
.plot $reluCheck 0.0 10.0
