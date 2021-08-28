<?php
    $filename = getenv("QUERY_STRING");
    $pos = strpos($filename, "=");
    if ($pos == false)
    {
        echo "Wrong request for download";
        exit ;
    }
    $realname = substr($filename, $pos + 1);
    $filepath = substr(getenv("HTTP_UPLOADS_PATH"), 1) . "/" . $realname;
    #echo $realname . " | " . $filepath . " | ";
    #$file = fopen($filepath, "r") or die("PHP: File not found");
    if (file_exists($filepath) == false)
    {
        echo "PHP: file not found";
        exit ;
    }
    
    #if (substr($realname, -strlen(".mp4")) === ".mp4")
    #{
    #    header('Content-Type: video/mp4');
    #    header("Content-Transfer-Encoding: binary");
    #}
    header('Content-Disposition: attachment; filename=' . $realname);
    #ob_clean();
    #flush();
    readfile($filepath);
    exit ;
    #while (!feof($file))
    #{
    #    echo fgets($file, 10485760);
    #}
    #fclose($file);
    
?>
