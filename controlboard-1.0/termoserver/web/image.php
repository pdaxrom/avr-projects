<?php

if ($stream = fopen('http://10.3.0.106:8081', 'r')) {

    while(true) {
	$data = fgets($stream, 256);

	$contentLength = 'unknown';

	if (preg_match('/Content-Length: .* (\d+)/', $data, $matches)) {
	    $contentLength = (int)$matches[1];
	    
	    $data = fgets($stream, 256);

	    $image = stream_get_contents($stream, $contentLength);
	    if ($image === false) {
		break;
	    }

//	    header("--BoundaryString");
	    header("Content-type: image/jpeg");
	    header("Content-Length: ".$contentLength);
	    echo $image;
	    break;
	}
    }

    fclose($stream);
}

?>
