## What is Captcha

CAPTCHA is an acronym for 
"Completely Automatic Public Turing Test to Tell Computers and Humans Apart".
It is a task, that human can easily solve, but computer not.

It is used as a challenge-response to ensure that the individual submitting 
information is a human and not an automated process. Typically, a captcha is 
used with form submissions where authenticated users are not necessary, 
but you want to prevent spam submissions.

## What is Hooto Captcha

hcaptcha is a free, easy-to-use WebService helps enterprises to integrate 
Captcha into their own business systems. Similar to Google reCaptcha but can 
be customized and deployment to your private network.

### Features
* High Security: using the distorted, bonding, dislocation and random word length, 
    making it difficult to be dismantled, recognition by machine
* High Performance: Based on C/libevent/libgd/memcached, all levels of optimization,
    upto 500 Requests pre second in single-threaded (120x60 pixels)
* Local Deployment: Similar to Google reCaptcha but can deployment to private network
* Cluster Deployment： Support for large-scale network deployment
    `LVS -> Hooto Captcha Cluster -> Memcached Cluster`

### Example
<table border="0">
  <tr>
    <td>
      <img src="/eryx/hcaptcha/raw/master/scripts/img/s1.png" />
    </td>
    <td>
      <img src="/eryx/hcaptcha/raw/master/scripts/img/s2.png" />
    </td>
    <td>
      <img src="/eryx/hcaptcha/raw/master/scripts/img/s3.png" />
    </td>
  </tr>
</table>

## Architecture Design

LVS -> Hooto Captcha Cluster -> Memcached Cluster

## Install

### Debian 6.x
    apt-get install gcc g++ libgd2-xpm-dev libevent-dev libmemcached-dev memcached git

### CentOS 5.x/6.x
    yum install gcc gcc-c++ gd-devel libevent-devel libmemcached-devel memcached git

### Download/Make
    git clone git://github.com/eryx/hcaptcha.git
    cd hcaptcha
    make
    make insall

### Start Server
    /opt/hcaptcha/bin/hcaptchad -c /opt/hcaptcha/etc/hcaptchad.conf
    Start hcaptcha/1.0.4 [OK]

### Stop Server
    killall hcaptchad
    Signal(15) Stop hcaptcha/1.0.4 [OK]

### Confirm the successful installation
    http://127.0.0.1:9527/hcaptcha/api/image?hcaptcha_token=123&hcaptcha_opt=refresh

## WebService API

### Display a CAPTCHA image    
    
* API Request  
URL  
    http://127.0.0.1:9527/hcaptcha/api/image  
Parameters (sent via GET)
<table>
    <tr>
        <td>hcaptcha_token (required)</td>
        <td>The random token created by client,  <br />
        Example: hcaptcha_token=abc123</td>
    </tr>
    <tr>
        <td>hcaptcha_opt (optional)</td>
        <td>Reload a new CAPTCHA image,  <br />
        Example: hcaptcha_opt=refresh</td>
    </tr>
</table>

* API Response

    Content-Type:image/png
    binary data...

### Verifying the User's Answer
    
After your page is successfully displaying CAPTCHA image, you need to configure
your form to check whether the answers entered by the users are correct.

* API Requset  
URL  
    http://127.0.0.1:9527/hcaptcha/api/verify  
Parameters (sent via GET)
<table>
    <tr>
        <td>hcaptcha_token (required)</td>
        <td>required. The random token created by client</td>
    </tr>
    <tr>
        <td>hcaptcha_word (required)</td>
        <td>The User's Answer</td>
    </tr>
</table>
    
* API Response  
The response from verify is a series of strings separated by "\n".  
To read the string, split the line and read each field.
<table>
    <tr>
        <td>Line 1</td>
        <td>"true" or "false".  <br />
        True if the CAPTCHA was successful</td>
    </tr>
    <tr>
        <td>Line 2</td>
        <td>if Line 1 is false,  <br />
        then this string will be an error code. CAPTCHA can display the error to the user/client  <br />
        </td>
    </tr>
</table>

### Error Code Reference
hcaptcha currently returns the following error codes:
<table>
    <tr>
        <td>incorrect-hcaptcha-word</td>
        <td>the user's answer was incorrect</td>
    </tr>
    <tr>
        <td>invalid-request</td>
        <td>the parameters of the verify was incorrect</td>
    </tr>
    <tr>
        <td>hcaptcha-not-reachable</td>
        <td>the hcaptcha service unavailable</td>
    </tr>
</table>
Example

    false
    incorrect-captcha-word

### Demo
    http://ws.hooto.com/hcaptcha/api/image?hcaptcha_token=123&hcaptcha_opt=refresh

## Reference
* http://www.captcha.ru/
* http://www.google.com/recaptcha/

