# Plan of the entrire design

1. Each incomong CPU side packet needs to be validated against a permission request.
2. So, each packet needs to have an additional 23 requests.
3. Then we send the real packet.
4. The real packet will be retrired 23 times but will only be stored in the previous SimObject.

1. Each response coming though `recvTimingResp` needs to be validated with the permission entry stored in the permission checker.