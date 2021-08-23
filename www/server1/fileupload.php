<?php
    function rus2translit($string) {
        $converter = array(
            '&#1071;' => 'a',   '&#1072;' => 'b',   '&#1073;' => 'v',
            '&#1074;' => 'g',   '&#1075;' => 'd',   '&#1076;' => 'e',
            '&#1077;' => 'e',   '&#1078;' => 'zh',  '&#1079;' => 'z',
            '&#1080;' => 'i',   '&#1081;' => 'y',   '&#1082;' => 'k',
            '&#1083;' => 'l',   '&#1084;' => 'm',   '&#1085;' => 'n',
            '&#1086;' => 'o',   '&#1087;' => 'p',   '&#1088;' => 'r',
            '&#1089;' => 's',   '&#1090;' => 't',   '&#1091;' => 'u',
            '&#1092;' => 'f',   '&#1093;' => 'h',   '&#1094;' => 'c',
            '&#1095;' => 'ch',  '&#1096;' => 'sh',  '&#1097;' => 'sch',
            '&#1098;' => '\'',  '&#1099;' => 'y',   '&#1100;' => '\'',
            '&#1101;' => 'e',   '&#1102;' => 'yu',  '&#1103;' => 'ya',

            '&#1040;' => 'A',   '&#1041;' => 'B',   '&#1042;' => 'V',
            '&#1043;' => 'G',   '&#1044;' => 'D',   '&#1045;' => 'E',
            '&#776;' => 'E',   '&#1046;' => 'Zh',  '&#1047;' => 'Z',
            '&#1048;' => 'I',   '&#1049;' => 'Y',   '&#1050;' => 'K',
            '&#1051;' => 'L',   '&#1052;' => 'M',   '&#1053;' => 'N',
            '&#1054;' => 'O',   '&#1055;' => 'P',   '&#1057;' => 'R',
            '&#1058;' => 'S',   '&#1059;' => 'T',   '&#1060;' => 'U',
            '&#1061;' => 'F',   '&#1062;' => 'H',   '&#1063;' => 'C',
            '&#1064;' => 'Ch',  '&#1065;' => 'Sh',  '&#1066;' => 'Sch',
            '&#1067;' => '\'',  '&#1068;' => 'Y',   '&#1069;' => '\'',
            '&#1070;' => 'E',   '&#1071;' => 'Yu',  '&#1072;' => 'Ya',
        );
        return strtr($string, $converter);
    }
    
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
    $filename2 .= str_replace(' ', '_', rus2translit(substr($content, $pos, $pos2 - $pos)));
    #$filename2 .= mb_substr($tmpstr, 0, strlen($tmpstr), "UTF-8");
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
    $result = fwrite($newfile, $newcontent);
    fclose($newfile);
    if ($result == false)
        echo "Error on file writing. Path: "  . "/uploads/" . $filename2;
    else
        echo "Uploadded successfully. File path: " . "/uploads/" . $filename2;
?>
