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

    if (file_exists($filepath) == false)
    {
        echo "PHP: file not found";
        exit ;
    }
    
    header('Content-Disposition: attachment; filename=' . $realname);
    readfile($filepath);
?>
