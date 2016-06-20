#!/usr/bin/env python3
import sys
import json
import urllib.request

def get_json(url):
    try: return json.loads(urllib.request.urlopen(url, timeout=12).read().decode('utf-8'))
    except urllib.error.HTTPError: return None
    else: return 'Request has timed out.'

def population(query):
    countries = get_json('http://api.population.io/1.0/countries')['countries']

    for country in countries:
        if query.lower() == country.lower():
            query = country.replace(' ', '%20')
            break

    url = 'http://api.population.io/1.0/population/2016/{}/'.format(query)
    res = get_json(url)

    if res is None: return print('Invalid country. Refer to http://api.population.io/1.0/countries for a list of all countries available.')
    elif isinstance(res, str) is True: return print(res)

    total   = 0
    males   = 0
    females = 0

    for entry in res:
        total   += entry['males'] + entry['females']
        males   += entry['males']
        females += entry['females']

    print('Total population ({}, 2016): {:,} | Males: {:,} ({}%) | Females: {:,} ({}%)'.format(
        query.replace('%20', ' '), total,
        males, round((males * 100)/total, 2),
        females, round((females * 100)/total, 2)
    ))

def weather(api_key, query):
    query = query.replace(' ', '%20')
    url = 'http://api.wunderground.com/api/{}/conditions/q/{}.json'.format(api_key, query)
    res = get_json(url)

    if isinstance(res, str) is True: return print(res)
    if 'response' in res:
        ptr = res['response']
        if 'results' in ptr: return print('Found {} results, please be more specific (i.e. "city country").'.format(len(ptr['results'])))
        if 'error' in ptr: return print(ptr['error']['description'] + '.')

    result = res['current_observation']
    print('Weather ({}): {} | Humidity: {} | Wind: {}\nLast updated on {}.'.format(
        result['display_location']['full'], result['temperature_string'],
        result['relative_humidity'],
        result['wind_string'],
        result['observation_time_rfc822']
    ))


if __name__ == '__main__':
    if len(sys.argv) == 3:
        if sys.argv[1] == '-w':
            weather('847c514d3a158eb4', sys.argv[2])
        elif sys.argv[1] == '-p':
            population(sys.argv[2])
    else:
        print('No query was given.')
