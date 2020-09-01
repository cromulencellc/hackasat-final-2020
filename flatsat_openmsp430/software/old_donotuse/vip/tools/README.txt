Several python scripts support message sending to the VIP badge. It is my intention to create one script that provides access to the rest of them but who knows if I'll get to that. This is sufficient to control the VIP badges for now. 


score_scraper.py - This script will pull the json score feed from a url and send to the badge server. May need to be modified for new URL or to work with whatever security stuff Vito puts on the scoreboard. 

send_news.py - This script will send a news update with whatever is on the command line. Behind the scenes it updates a counter (stored in newscount.dat). Running ./send_news.py clear will increment the news counter sufficiently to clear out all news displayed on the badge. If you run this twice before the server actually transmits the message, the first message will be lost (Server does not queue outgoing messages). On the badge each message is limited to 26 characters long (the rest is truncated). If I get time I'll add line wrap to my script...
	Ex. ./send_news.py This is a news update
		Badge will display:
			This is a news update

		./send_news.py This is the next news update
		Badge will display:
			This is the next news update
			This is a news update

		./send_news.py clear
		Badge will display:


update_string.py - This will send a message that updates a display string on the badge. Display strings are used for page headings ("DEF CON 22 CTF") and informational messages ("Stale scoreboard. Proceed to CTF Room"), etc.  It is not likely that these will change very often, but the functionality exists just for the hell of it. The default strings are stored in vip_strings_reboot.dat (The badge holds these strings after a reboot). And the modified strings that have been sent are stored in vip_strings.dat. These files are handled by the script, not the user. 
		./update_string.py 1 "CTF SCOREBOARD"
		Updates the title on the scoreboard page. 
		./update_string 2 "GET YOUR ASS\nTO THE CTF ROOM"
		Updates the info message when the scoreboard is invalid



vip_message.py - Support library for message sending. You will need to modify VIP_SERVER with the IP/url for the badge server client on the game network. 