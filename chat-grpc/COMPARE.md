For static requests, there is a general trend of dyde being at most half the size of JSON. Some requests have bodies that are dynamic/depend on the content, but even for extreme ones like GetConversations, we can see that dyde is much more efficient with at least a 100 byte difference.

This makes sense as JSON serialization needs to have its special markers to support generic parameters, but dyde's serialization can efficiently pack parameters together since all the parameters are known on both the client and server, whereas the JSON essentially has to include its "type" information in its serialization for all of its parameters.

### dyde

- SigninUser

  - 31 ("bob", "bob")
  - 10

- SignupUser

  - 33 ("cate", "cate")
  - 10

- SignoutUser

  - 17
  - 2

- DeleteUser

  - 29
  - 2

- CreateConversation

  - 25
  - 30

- GetConversations
  - 17
  - 154 (4 conversations)

### JSON

- SigninUser

  - 67 ("bob", "bob")
  - 43

- SignupUser

  - 69 ("cate", "cate")
  - 48

- SignoutUser

  - 43
  - 36

- DeleteUser

  - 63
  - 36

- CreateConversation

  - 67
  - 101

- GetConversations
  - 43
  - 253 (3 conversations)
