
the server prints

 > ./server
accept: Software caused connection abort


Aas you see, it is the same as by using the scheme wrappers.

Another 'bug' from netinet/sctp_uio.h:

int     sctp_sendmsg    __P((int, void *, size_t, struct sockaddr *,
         socklen_t, uint32_t, uint32_t, uint16_t, uint32_t, uint32_t));

should read

int     sctp_sendmsg    __P((int, const void *, size_t, struct sockaddr 
*,
         socklen_t, uint32_t, uint32_t, uint16_t, uint32_t, uint32_t));

the second arg should be const.

I hope you can reproduce the problem with the example programs.

Best regards
Michael

On Feb 29, 2004, at 9:44 PM, Randall R. Stewart (home) wrote:

> Michael Tüxen wrote:
>
>> Randy,
>>
>> that is nice.
>>
>> I have now guile extended to support (at a minimum) SCTP,
>> that is all constants are known and the sctp_sendmsg function
>> is supported.
>>
>> I have two problems: first, libsctp.a is a static library and
>> it is not good to link dynamic libraries like libguile against
>> static ones. Is it possible to add a dynamic libsctp to the
>> KAME stuff?
>
>
> Sure.. That is just a matter of me hunting down the
> options and changing the make file to creat a .so and a .a....
>
> I will add that to my "todo" list too :->
>
>>
>>
>> The second problem is the following:
>>
>> The server runs:
>>
>> guile> (define s (socket AF_INET SOCK_STREAM IPPROTO_SCTP))
>> guile> (bind s AF_INET (inet-aton "127.0.0.1") 2000)
>> guile> (listen s 1)
>> guile> (define ss (accept s))
>>
>> (please note that all scheme socket related functions just map
>>  to the C-based system calls, they are just wrapper)
>>
>> Now the accept call is blocking as expected.
>>
>> The server does:
>
> you mean client, I think?
>
>>
>> (define s (socket AF_INET SOCK_SEQPACKET IPPROTO_SCTP))
>> (sctp-sendmsg s "A" 0 0 0 0 AF_INET (inet-aton "127.0.0.1") 2000 
>> MSG_EOF)
>>
>> I would expect now, that the accept call returns. But what happens
>> is that accept returns with an error:
>> <unnamed port>:6:12: In procedure accept in expression (accept s):
>> <unnamed port>:6:12: Software caused connection abort
>> ABORT: (system-error)
>
> Hmm.. I am unfamiliar with the output of teh above... I am surprised
> that the cookie and data did not  bundle.. they should have. As to
> the specific errors... I cannot tell... Since the data went across
> the wire and all looked well.. (with the exception of the unexpected
> non-bundle of data and cookie) I am at a loss as to what caused the
> "ABORT".  Maybe you could insert some debug somewhere to
> see what the "errno" if anything is being set to... there must
> be a system call failing... or something...
>
> As to the bundling.. ahh.. wait this might be explained by the fact 
> that
> it is localhost. I have seen cases where the INIT/INIT-ACK are being
> exchanged while the data is still being queued... since the packets
> traverse so quickly... not sure though .. this could be a strech.. you
> could try getting
> SCTP_DEBUG
> defined and then use
> sctp_test_app
>
> to:
> setdebug all
>
> Hmm on thinking about this.. you probably don't have a copy of
> the sctp_test_app tool.. maybe you should checkout the
> cvs repository.. then you do a
> gmake all
>
> in the top of the tree and you will get under apps/FreeBSD
> a sctp_test_app that will allow you to set debug...
>
> I am at a loss as to why things are not working.. not without
> more information or at least an error from something with
> an error code (errno)..
>
>
>>
>>
>> Looking at the ethereal trace I see a normal association setup,
>> DATA in a  separate packet, SACK backwards and then the client
>> starts the shutdown procedure. So on the wire everything is OK
>> (I would have expected that the DATA chunk is bundled with
>> the COOKIE-ECHO, but it isn't).
>>
>> This seems to be a bug, right?
>
>
> Yes.. its a bug somewhere.. it may be in the SCTP code.. but then
> again I don't know where to look without more info..
>
> R
>
>>
>>
>> Best regards
>> Michael
>>
>>
>> On Feb 29, 2004, at 6:09 PM, Randall R. Stewart (home) wrote:
>>
>>> Michael:
>>>
>>> Ok, I have the changes made... so you will no
>>> longer need to zero out the struct.. however it won't
>>> go in until our next big KAME patch... it will have
>>> all the fixes for the new kame re-structure they are doing.
>>>
>>> Since you have CVS access if you ever want to
>>> get the current code you can do so by doing a
>>>
>>> CVSROOT=stewart.chicago.il.us:/home/sctpBsd
>>>
>>> However, when you check it out you must do so by list.. there
>>> is a "cisco" directory in there that will stop a
>>> cvs checkout .
>>>
>>> We need to move this to its own repository :-0
>>>
>>> Hope all is well for you there..
>>>
>>> R
>>>
>>>>
>>>>
>>>
>>>
>>> -- 
>>> Randall R. Stewart
>>> 815-477-2127 (office)
>>> 815-342-5222 (cell phone)
>>>
>>>
>>>
>>
>>
>>
>
>
> -- 
> Randall R. Stewart
> 815-477-2127 (office)
> 815-342-5222 (cell phone)
>
>
>




