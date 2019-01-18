# HTTP Web Server - Web crawler
### Markos Varvagiannis
#### sdi1400017@di.uoa.gr

## Compile:
Τα προγράμματα έχουν χωριστεί σε **ξεχωριστούς φακέλους**, με τη λογική να είναι *ανεξάρτητα*, καθώς μπορεί να τρέχουν και σε διαφορετικούς υπολογιστές. Ως εκ τούτου το κάθε πρόγραμμα έχει το δικό του Makefile.

Ωστόσο, υπάρχει και η δυνατότητα μεταγλώτισης και των δύο με το εξωτερικό Makefile που περιέχεται στον φάκελο.



## Run:
### Web Server
	./myhttpd -p serving_port -c command_port -t num_of_threads -d root_dir

### Web Crawler
	./mycrawler -h host_or_IP -p port -c command_port -t num_of_threads -d save_dir starting_URL
## Σημειώσεις Υλοποίησης:
* Η άσκηση είναι γραμμένη σε γλώσσα **C**.
* Για τις ουρές, χρησιμοποιήθηκε η τεχνική της **απόκρυψης** για λόγους abstraction, αλλά κυρίως προστασίας. Οι δηλώσεις των structs δηλαδή περιέχονται στα πηγαία αρχεία και όχι στα αρχεία κεφαλίδας.
* Δεν έχει υλοποιηθεί η δυνατότητα search, καθώς και το προαιρετικό κομμάτι επαναδημιουργίας των threads σε περίπτωση που τερματίσουν.
