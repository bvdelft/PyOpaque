# TODO

 - documentation of the API
 - dir / dict features (docstring)
 - example policies
 - scenarios (examples)
   - Classes needing private fields
     - E.g. A class that has a field 'name' which will end up in an SQL query 
       only allows this field to be set via a special setter that removes quotes
   - Instances needing private fields
     - E.g. A program delegates a token to perform a certain operation, but only
       for a limited number of times (the code receiving the delegated token 
       should not be able to get the real token)
 - explore memory leakage
