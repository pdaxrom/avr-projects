<html>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<meta http-equiv="Content-Language" content="ru"/> 
<meta http-equiv="refresh" content="1" />
<body>
<img src="image.php"/>
<br/>
<br/>
Температура в офисе: 
<?php
$temp = system("nc 10.3.0.106 8082", $retval);
echo $temp;
?>
C
<br/>
</body>
</html>
