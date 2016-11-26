<?php
header('Access-Control-Allow-Origin: *');
header('Content-Type: application/json');
echo readfile("city.json");
?>