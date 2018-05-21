#pragma once

class ICycledInterface
{
public:
   virtual void Initialize(void) = 0;
   virtual void Update(void) = 0; // Should it have time?
   virtual void Shutdown(void) = 0;
};

typedef ICycledInterface IInterface; // REDUNDANCY
