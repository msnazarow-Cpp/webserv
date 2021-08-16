<?php
    $content = "";
    $newcontent = "";
    $boundary = "";
    $date = date_create();
    $filename = getenv("HTTP_TMP");
    $filename2 = substr($filename, 9) . "_" . date_timestamp_get($date) . "_";
    $length = getenv("CONTENT_LENGTH");
    $file = fopen($filename, "r+");
    if($file == false)
    {
        echo "PHP: File uploading error";
        return ;
    }
    
    while (!feof($file))
    {
        $buffer = fgets($file, 12288);
        $content .= $buffer;
    }
    fclose($file);
    
    $pos2 = strpos($content, "\r\n");
    if ($pos2 == false)
    {
        echo "PHP: File uploading error";
        return ;
    }
    $boundary = substr($content, 0, $pos2);
    #echo "BOUNDARY: " . $boundary;
    
    $pos = strpos($content, "filename=\"", $pos2);
    if ($pos == false)
    {
        echo "PHP: File uploading error";
        return ;
    }
            
    $pos = $pos + 10;
    $pos2 = strpos($content, "\"", $pos);
    if ($pos2 == false)
    {
        echo "PHP: File uploading error";
        return ;
    }
    $filename2 .= substr($content, $pos, $pos2 - $pos);
    #echo "FILENAME: " . $filename2;
        
    $pos = strpos($content, "\r\n\r\n", $pos2);
    if ($pos == false)
    {
        echo "PHP: File uploading error";
        return ;
    }
    $pos = $pos + 4;
    $length = $length - $pos;
    $pos2 = strpos($content, $boundary, $pos);
    if ($pos2)
    {
        $length = $pos2 - $pos - 2;
    }
    $newcontent = substr($content, $pos, $length);
    $newfile = fopen("uploads/" . $filename2, "w") or die("PHP: File uploading error");
    fwrite($newfile, $newcontent);
    fclose($newfile);
    echo "Uploadded successfully";
?> #Тут не было закрывающей скобки