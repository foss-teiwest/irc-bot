#!/usr/bin/env python3
import sys

def main(api_key):
    if len(sys.argv) == 3: query = sys.argv[2].replace(' ', '%20')
    else: return print("No query was given.")

    opening = "<plaintext>"
    closure = "</plaintext>"

    # weather
    if sys.argv[1] == '-w':
        import urllib.request
        from socket import timeout

        url = "http://api.wolframalpha.com/v1/query?appid={}&input=weather%20{}".format(api_key, query)

        try: resp = str( urllib.request.urlopen(url, timeout=12).read() )
        except timeout: return print("Request timed out")

        title_hook = resp.find(opening)
        if title_hook != -1:
            title = resp[title_hook+11:resp.find(closure)] + "\n"
            hook = resp.find('"InstantaneousWeather:WeatherData"')
            result = resp[resp.find(opening, hook)+11:resp.find(closure, hook)]
            # cleaning up result because urllib...
            result = result.replace(" \\xc2\\xb0C", "°").replace("\\n", "\n")
            print(title.replace(" |", ":") + result.replace(" |", ":"))
        else:
            print("Not found.")

    # population
    elif sys.argv[1] == '-p':
        import urllib.request
        from socket import timeout

        url = "http://api.wolframalpha.com/v1/query?appid={}&input=population%20{}".format(api_key, query)

        try: resp = str( urllib.request.urlopen(url, timeout=12).read() )
        except timeout: return print("Request timed out")

        title_opening = resp.find(opening)
        if title_opening != -1:
            title_closure = resp.find(closure)
            title = resp[title_opening+11:title_closure] + "\n"
            result = resp[resp.find(opening, title_opening+1)+11:resp.find(closure, title_closure+1)]
            print(title.replace(" |", ":") + result)
        else:
            print("Not found.")

    else:
        print("No valid argument found.")

if __name__ == '__main__':
    api_key = "J6HA6V-YHRLHJ8A8Q"
    main(api_key)
