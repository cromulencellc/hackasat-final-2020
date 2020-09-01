The server client runs on the server laptop that is connected over a UART to the
server badge.

The server client will handle outside network requests from the Scorebot

There are two types of network requests from the scorebot:

SLA availability polls
Token deposits

The token deposit will just pass the round number for the next tokens.
This round number will be used to generate the tokens for the next round
by the server -- This number needs to be used by the badges to generate a new
scoring token.

The badge runs at 5 minute rounds (perfectly) it is important that we maintain round
control from the scorebot.
So if the scorebot takes longer than 5 minutes we will get out of sync and there will
be a scenario immediately the next round where they can submit the same token twice.

--SirGoon
