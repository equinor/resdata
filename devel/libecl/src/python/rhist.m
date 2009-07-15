function [no,xo] = rhist(varargin)
%RHIST   Relative Histogram.
%   N = HIST(Y) bins the elements of Y into 10 equally spaced containers
%   and returns the relative frequency of elements in each container. If Y is a
%   matrix, RHIST works down the columns.
%
%   N = RHIST(Y,M), where M is a scalar, uses M bins.
%
%   N = RHIST(Y,X), where X is a vector, returns the relative freqency of Y
%   among bins with centers specified by X. The first bin includes
%   data between -inf and the first center and the last bin
%   includes data between the last bin and inf. Note: Use HISTC if
%   it is more natural to specify bin edges instead. 
%
%   N = RHIST(Y,M,Any_Character) returns relative frequency density of
%   Y among bins.Any_Character is the any character inside single quotation
%   or any numeric value.
%   You can omit second optional argument using single quotation 
%   i.e. N = RHIST(Y,'',Any_Character) returns relative frequency density
%   for 10 bins.
%   It is to be noted that sum(N)equals unity for relative frequency 
%   while area under curve for relative frequency density equals unity.
%   Note that as size(Y,1) and M increases relative frequency density is
%   close to probability density for continous random variable.
%
%   [N,X] = RHIST(...) also returns the position of the bin centers in X.
%
%   RHIST(...) without output arguments produces a histogram of relative 
%   frequency or relative frequency densisty bar plot of the results. 
%   The bar edges on the first and last bins may extend to cover the min 
%   and max of the data unless a matrix of data is supplied.
%
%   RHIST(AX,...) plots into AX instead of GCA.
%
%   Class support for inputs Y, X: 
%      float: double, single
%
%   See also HIST.

%   Copyright 2004-2005 Durga Lal Shrestha. 
%   $Revision: 1.0.0 $  $Date: 2005/6/20 14:30:00 $

% Parse possible Axes input

error(nargchk(1,inf,nargin));
[cax,args,nargs] = axescheck(varargin{:});

y = args{1};
if nargs == 1
    x = 10;
elseif nargs == 2
    x = args{2};
else
    if isempty(args{2})
        x = 10;
    else
        x = args{2};
    end
end
[m,n] = size(y);
[nn,x]=hist(y,x); % frequency
nn = nn./m;     % relative frequency

%  relative frequency density
if nargs == 3 
    binwidth = x(2)-x(1);
    nn = nn./binwidth;  
end

if nargout == 0
  if ~isempty(cax)
    bar(cax,x,nn,[min(y(:)) max(y(:))],'hist');
  else
    bar(x,nn,[min(y(:)) max(y(:))],'hist');
  end
  xlabel('y')
  if nargs == 3            
    ylabel('relative frequency density')
  else
     ylabel('relative frequency')
  end        
else
  no = nn;
  xo = x;  
end
