<?php
// $Id$

/**
 * @file ting.client.inc
 * Wrapper functions for Ting client.
 */

// Ting client has a ton of includes. Let's load them.
$includes = array(
  'ting-client/lib/TingClient.php',
  'ting-client/lib/adapter/http/TingClientDrupal6HttpRequestAdapter.php',
  'ting-client/lib/request/rest-json/RestJsonTingClientRequestFactory.php',
  'ting-client/lib/log/TingClientDrupalWatchDogLogger.php',
  'addi-client/AdditionalInformationService.php'
);

$path = drupal_get_path('module', 'ting') . '/lib/';

foreach ($includes as $include) {
  require_once($path . $include);
}

/**
 * Load an object from Ting by ID.
 *
 * @param string $object_id
 *    Ting object ID.
 * @param bool $enrich
 *    Whether to enrich the object with additional information, covers etc.
 * @return mixed
 *    Object, if found - boolean FALSE if not.
 */
function ting_get_object_by_id($object_id, $enrich = FALSE) {
  static $cache = array();

  if (isset($cache[$object_id])) {
    $object = $cache[$object_id];
  }
  else {
    $request = ting_get_request_factory()->getObjectRequest();
    $request->setObjectId($object_id);
    $request = ting_add_agency($request);
    $object = ting_execute($request);

    if ($object) {
      $object = ting_add_object_info($object);
    }
  }

  if ($object && $enrich && empty($object->enriched)) {
    $object = array_shift(ting_add_additional_info(array($object)));
    $object->enriched = TRUE;
  }

  $cache[$object_id] = $object;

  return $object;
}

function ting_get_object_by_isbn($isbn, $enrich = FALSE) {
	/* isbn conversion to isbn13 */
	$isbn = convertToEAN($isbn);
	
	static $cache = array();

  if (isset($cache[$isbn])) {
    $object = $cache[$isbn];
  }
  else {
    $request = ting_get_request_factory()->getObjectRequest();
    $request->setISBN($isbn);
    $request = ting_add_agency($request);
    $object = ting_execute($request);

    if ($object) {
      $object = ting_add_object_info($object);
    }
  }

  if ($object && $enrich && empty($object->enriched)) {
    $object = array_shift(ting_add_additional_info(array($object)));
    $object->enriched = TRUE;
  }

  $cache[$isbn] = $object;

  return $object;
}

/**
 * Load an object from Ting by local ID (faust number).
 *
 * @param string $local_id
 *    Local identifier
 * @param bool $enrich
 *    Whether to enrich the object with additional information, covers etc.
 * @return TingClientObject
 */
function ting_get_object_by_local_id($local_id, $enrich = false) {
	$request = ting_get_request_factory()->getObjectRequest();
	$request->setLocalId($local_id);
	$request = ting_add_agency($request);
	$object = ting_execute($request);

	if ($object) {
		$object = ting_add_object_info($object);
		$object = ($enrich) ? array_shift(ting_add_additional_info(array($object))) : $object;
	}
	return ($object) ? $object : false;
}

/**
 * Display a Ting collection of objects.
 *
 * @param string $collection_id
 *    Ting collection ID.
 * @param bool $enrich
 *    Whether to enrich objects in the collection with additional information, covers etc.
 * @return mixed
 *    Collection object, if found - boolean FALSE if not.
 */
function ting_get_collection_by_id($object_id, $enrich = FALSE) {
  static $cache = array();

  if (isset($cache[$object_id])) {
    $collection = $cache[$object_id];
  }
  else {
    $request = ting_get_request_factory()->getCollectionRequest();
    $request->setObjectId($object_id);
    $request = ting_add_agency($request);
    $collection = ting_execute($request);

    if ($collection) {
      $collection = ting_add_collection_info($collection);
    }
  }

  if ($collection && $enrich && empty($collection->enriched)) {
    $collection = ting_add_additional_info($collection);
    $collection->enriched = TRUE;
  }

  $cache[$object_id] = $collection;

  return $collection;
}

/**
 * Return the possible sort keys and their corresponding labels.
 *
 * @return
 *   A key/value array of sort keys and labels.
 */
function ting_search_sort_options() {
  return array(
    '' => t('Relevance'),
    'title_ascending' => t('Title – A → Z'),
    'title_descending' => t('Title – Z → A'),
    'creator_ascending' => t('Author – A → Z'),
    'creator_descending' => t('Author – Z → A'),
    'date_descending' => t('Year of publication – newest first'),
    'date_ascending' => t('Year of publication – oldest first'),
  );
}

/**
 * Performs a search agains the 
 *
 * @param string $query
 *    The search query
 * @param int $page
 *    The page number to retrieve search results for
 * @param int $resultsPerPage
 *    The number of results to include per page
 * @param array $options
 *    Options to pass to the search. Possible options are:
 *    - facets: Array of facet names for which to return results. Default: facet.subject, facet.creator, facet.type, facet.date, facet.language
 *    - numFacets: The number of terms to include with each facet. Default: 10
 *    - enrich: Whether to include additional information and cover images with each object. Default: false
 *    - sort: The key to sort the results by. Default: "" (corresponds to relevance). The possible values are defined by the sortType type in the XSD.
 *    - supportingTypes: Whether to include supporting types such as reviews. Default: false
 * @return TingClientSearchResult
 *    The search result
 */
function ting_do_search($query, $page = 1, $resultsPerPage = 10, $options = array()) {
	$request = ting_get_request_factory()->getSearchRequest();
	$request->setQuery($query);
	$request = ting_add_agency($request);
	$request->setStart($resultsPerPage * ($page - 1) + 1);
	$request->setNumResults($resultsPerPage);
  
	$request = (!isset($options['supportingTypes']) || !$options['supportingTypes']) ? ting_add_supporting_types($request) : $request;
	$request->setFacets((isset($options['facets'])) ? $options['facets'] : array('facet.subject', 'facet.creator', 'facet.type', 'facet.date', 'facet.language'));
	$request->setNumFacets((isset($options['numFacets'])) ? $options['numFacets'] : ((sizeof($request->getFacets()) == 0) ? 0 : 10));
  $request->setSort((isset($options['sort'])) ? $options['sort'] : '');
  $request->setAllObjects(isset($options['allObjects']) ? $options['allObjects'] : FALSE);

	$searchResult = ting_execute($request);

	//Decorate search result with additional information
	foreach ($searchResult->collections as &$collection) {
		$collection = ting_add_collection_info($collection);
		if (isset($options['enrich']) && $options['enrich']) {
			$collection = ting_add_additional_info($collection);
		}
	}

	return $searchResult;
}

/**
 * @param string $query The prefix to scan for
 * @param int $numResults The numver of results to return
 * @return TingClientScanResult
 */
function ting_do_scan($query, $numResults = 10) {
	$request = ting_get_request_factory()->getScanRequest();
	$request->setField('phrase.anyIndexes');
	$request->setLower($query);
	$request->setNumResults($numResults);
	return ting_execute($request);
}

/**
 * @param string $word The word to get spell suggestions for
 * @param $numResults The number of results to return
 * @return array An array of TingClientSpellSuggestion objects
 */
function ting_get_spell_suggestions($word, $numResults = 10) {
	$request = ting_get_request_factory()->getSpellRequest();
	$request->setWord($word);
	$request->setNumResults($numResults);
	return ting_execute($request);
}

/**
 * @param string $isbn ISBN number to get recommendations from
 * @param $numResults The number of results to return
 * @return array An array of TingClientObjectRecommendation objects
 */
function ting_get_object_recommendations($isbn, $numResults = 10) {
	$request = ting_get_request_factory()->getObjectRecommendationRequest();
	$request->setIsbn($isbn);
	$request->setNumResults($numResults);
	return ting_execute($request);
}

/**
 * Perform a request against Ting and perform error handling if necessary
 * 
 * @param $request The request
 * @return mixed Result of the request or false if an error occurs 
 */
function ting_execute($request) {
	try {
  	return ting_get_client()->execute($request);
	} catch (TingClientException $e) {
	  watchdog('ting client', 'Error performing request: '.$e->getMessage(), NULL, WATCHDOG_ERROR, 'http://'.$_SERVER["HTTP_HOST"].$_SERVER["REQUEST_URI"]);
		return false;
	}
}

/**
 * Retrieves an initialized Ting client with appropriate request adapter and logger 
 * 
 * @return TingClient
 */
function ting_get_client() {
	static $client;

	if (!isset($client))
	{
		$logger = (variable_get('ting_enable_logging', false)) ? new TingClientDrupalWatchDogLogger() : new TingClientVoidLogger();
		$client = new TingClient(new TingClientDrupal6HttpRequestAdapter(), $logger);
	}

	return $client;
}

/**
 * Retrieves an initialized Ting client request factory.
 * 
 * @return TingClientRequestFactory
 */
function ting_get_request_factory() {
	static $requestFactory;

	if (!isset($requestFactory))
	{
		$urlVariables = array(	'search' => 'ting_search_url',
													 	'scan' => 'ting_scan_url',
														'object' => 'ting_search_url',
														'collection' => 'ting_search_url',
														'spell' => 'ting_spell_url',
														'recommendation' => 'ting_recommendation_server');
			
		$urls = array();
		foreach ($urlVariables as $name => $setting)
		{
			$urls[$name] = variable_get($setting, false);
			if (!$urls[$name]) {
				throw new TingClientException('No Ting webservice url defined for '.$name);
			}
		}

		$requestFactory = new RestJsonTingClientRequestFactory($urls);
	}

	return $requestFactory;
}

function ting_add_collection_info(TingClientObjectCollection $collection) {
  $types = array();
  $subject_count = array();
  $common_object = NULL;
  // Sort by type and language with most probable results first, then use the
  // abstract of the first object with an abtract.
  usort($collection->objects, 'ting_compare_objects');
  foreach ($collection->objects as $object) {
    $object = ting_add_object_info($object);
    $types[] = $object->type;
    foreach ($object->subjects as $subject) {
      if (!isset($subject_count[$subject])) {
        $subject_count[$subject] = 0;
      }
      $subject_count[$subject]++;
    }
    if (!$common_object && $object->abstract) {
      $common_object = $object;
    }
  }
  $collection->types = array_unique($types);
  asort($subject_count);
  // All subjects from all objects with the most common ones first.
  $collection->subjects = array_keys($subject_count);
  if (!$common_object) {
    $common_object = $collection->objects[0];
  }

  $collection->id = $common_object->id;
  $collection->title = $common_object->title;
  $collection->abstract = $common_object->abstract;
  $collection->creators = $common_object->creators;
  $collection->creators_string = $common_object->creators_string;
  $collection->date = $common_object->date;

  // For single-object collections, we skip the collection page.
  $collection->url = count($collection->objects) == 1 ? $collection->objects[0]->url : url('ting/collection/' . $collection->id, array('absolute' => true));

  return $collection;
}

function ting_add_object_info(TingClientObject $object) {
  $object->type = $object->record['dc:type']['dkdcplus:BibDK-Type'][0];
  $object->language = !empty($object->record['dc:language'][''][0]) ? $object->record['dc:language'][''][0] : FALSE;

  $object->title = $object->record['dc:title'][''][0];
  $object->abstract = !empty($object->record['dcterms:abstract'][''][0]) ? $object->record['dcterms:abstract'][''][0] : FALSE;
  $object->date = !empty($object->record['dc:date'][''][0]) ? $object->record['dc:date'][''][0] : FALSE;

  $object->creators = array();
  if (!empty($object->record['dc:creator'])) {
    foreach ($object->record['dc:creator'] as $type => $dc_creator) {
      if ($type != 'oss:sort') {
        $object->creators = array_merge($object->creators, $dc_creator);
      }
    }
  }
  $object->creators_string = implode(', ', $object->creators);

  $object->subjects = array();
  if (!empty($object->record['dc:subject'])) {
    foreach ($object->record['dc:subject'] as $type => $dc_subject) {
      if (in_array($type, array('dkdcplus:DBCF', 'dkdcplus:DBCS', 'dkdcplus:DBCM', 'dkdcplus:DBCO', 'dkdcplus:DBCN'))) {
        $object->subjects = array_merge($object->subjects, $dc_subject);
      }
    }
  }

	$object->url = url('ting/object/'.$object->id, array('absolute' => TRUE));

  if ($object->type == 'Netdokument') {
    foreach ($object->record['dc:identifier']['dcterms:URI'] as $url) {
      $object->online_url = ting_rewrite_download_url($url);
    }
  }

	return $object;
}

/**
 * Add additional information info for cover images.
 *
 * @param object $collection
 * @return object
 */
function ting_add_additional_info($collection) {
	$faust_numbers = array();

	$objects = (isset($collection->objects)) ? $collection->objects : $collection;

  foreach ($objects as $object) {
    if ($object->localId) {
      $faust_numbers[] = $object->localId;
    }
  }
	
  if (sizeof($faust_numbers) > 0) {
    foreach (ting_get_additional_info($faust_numbers) as $faust_number => $ai) {
      foreach ($objects as &$object) {
        if ($faust_number == $object->localId) {
          $object->additionalInformation = $ai;
        }
      }
    }
  }

  if (isset($collection->objects)) {
    $collection->objects = $objects;
  }
  else {
		$collection = $objects;
  }

  return $collection;
}

/**
 * Get additional info for a number of FAUST numbers.
 *
 * @param array $faust_numbers
 *    Array of the FAUST numbers to look up.
 * @return array
 *    Additional info keyed by FAUST number.
 */
function ting_get_additional_info($faust_numbers) {
  $addiVariables = array(
    'wsdlUrl' => 'addi_wdsl_url',
    'username' => 'addi_username',
    'group' => 'addi_group',
    'password' => 'addi_password',
  );

	foreach ($addiVariables as $name => &$setting) {
		$setting = variable_get($setting, false);
		if (!$name) {
			watchdog('TingClient', 'Additional Information service setting '.$name.' not set', array(), WATCHDOG_WARNING);
			return $collection;
		}
	}

	extract($addiVariables);

	$additionalInformationService = new AdditionalInformationService($wsdlUrl, $username, $group, $password);

  try {
    return $additionalInformationService->getByFaustNumber($faust_numbers);
  }
  catch (AdditionalInformationServiceException $e) {
    watchdog('TingClient', 'Error retrieving additional information and covers: '.$e->getMessage(), $isbns, WATCHDOG_ERROR);
  }
  // TODO: SoapFault should probably be catched within the Ting client.
  catch (SoapFault $e) {
    watchdog('TingClient', 'Error retrieving additional information and covers: '.$e->getMessage(), $isbns, WATCHDOG_ERROR);
  }

  return array();
}

/**
 * Callback for usort().
 *
 * Sorts objects according to "ORDER BY language_value, type_value", with
 * language_value being Danish=1, English=2, Other=3, and type_value being
 * Book=1, DVD=2, Other=3.
 */
function ting_compare_objects($o1, $o2) {
  $criteria = array(
    array(array('dc:language', 'dcterms:ISO639-2'), array('dan', 'eng')),
    array(array('dc:type', 'dkdcplus:BibDK-Type'), array('Bog', 'DVD')),
  );
  foreach ($criteria as $criterion) {
    list($keys, $values) = $criterion;
    list($key1, $key2) = $keys;
    $i1 = !empty($o1->record[$key1][$key2]) ? array_search($o1->record[$key1][$key2][0], $values) : FALSE;
    $i2 = !empty($o2->record[$key1][$key2]) ? array_search($o2->record[$key1][$key2][0], $values) : FALSE;
    if ($i1 !== FALSE && $i2 !== FALSE) {
      // Both objects has one of the specified values for this criteria (e.g.
      // when comparing "Bog" to "DVD").
      if ($i1 != $i2) {
        // The values were identical; move on to next criteria.
        return $i1 - $i2;
      }
    }
    elseif ($i1 !== FALSE) {
      // Only the first object has one of the specified values (e.g. when
      // comparing "Bog" to "VHS").
      return -1;
    }
    elseif ($i2 !== FALSE) {
      // Only the last object has one of the specified values (e.g. when
      // comparing "Bog" to "Bog").
      return 1;
    }
  }
  return 0;
}

function ting_add_agency(TingClientAgentRequest $request) {
	if ($agency = variable_get('ting_agency', false))
	{
		$request->setAgency($agency);
	}
	return $request;
}

/**
* Add checked materials from ting.admin, to the request query
*/
function ting_add_supporting_types(TingClientSearchRequest $request) {
  $supportingTypes = arrange_types(variable_get('ting_materials', array()));
  if(!empty($supportingTypes))
  {
  	foreach ($supportingTypes as &$type)
  	{
  		$type = 'facet.type="' . $type . '"';
  	}
  	$request->setQuery($request->getQuery().' ('.implode(' OR ', $supportingTypes).')');
  }
	return $request;
}

/**
 * Rewrite the download URL of external restricted-access ressources.
 *
 * This allow access to the ressources through the library's proxy server.
 *
 * Only URLs refererring to sites specified on the Ting settings page are
 * modified; other URLs are returned unmodified.
 */
function ting_rewrite_download_url($url) {
  $host = parse_url($url, PHP_URL_HOST);
  if (in_array($host, variable_get('restricted_access_proxy_hostnames', array()))) {
    $prefix = variable_get('restricted_access_proxy_prefix', '');
    if ($prefix) {
      $url = $prefix . $url;
    }
  }
  return $url;
}

/**
* Create an array of the chosen materials from ting.admin
* 
* @param array $types
* @return array
*/
function arrange_types($types = false) {
  $return = array();
  if (is_array($types)) {
    foreach ($types as $k => $s) {
      if ($s == true) {
        $return[] = $k;
      }
    }
  }
  return $return;
}
