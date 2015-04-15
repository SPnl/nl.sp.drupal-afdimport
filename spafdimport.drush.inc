<?php

// ini_set('display_errors','on');
// error_reporting(E_ALL);

// Implements hook_drush_command
function spafdimport_drush_command() {
	$items = array();

	$items['afdimportnew'] = array(
		'description' => "SP Afdelingen Import (new)",
		'aliases'     => array('afdnew'),
        );

	return $items;
}

function drush_spafdimport_afdimportnew() {
	require_once dirname(__FILE__) . '/spafdimport.inc';
	return spafdimport_afdelingen();
}
